#pragma once

#include<type_traits>
#include"core/type.h"
#include"core/util/enum_as_flag.h"

PROJECT_NAMESPACE_BEGIN

// is base type, byte size is sizeof(T)
template<typename T,  typename R = std::remove_reference_t<T>>
struct is_serializible_base_type
    : std::bool_constant< std::is_pod_v<R>  /*std::is_integral_v<R> || std::is_enum_v<R> || std::is_floating_point_v<R>*/ >
{};

template<typename T>
constexpr bool is_serializible_base_type_v = is_serializible_base_type<T>::value;


template<typename T,  typename R = std::remove_reference_t<T>> struct is_std_string:public std::false_type{};
template<typename T> struct is_std_string<T, std::string>:public std::true_type{};
template<typename T, typename R> struct is_std_string<T const,R>:public is_std_string<T>{};
template<typename T, typename R> struct is_std_string<T volatile,R>:public is_std_string<T>{};
template<typename T, typename R> struct is_std_string<T const volatile,R>:public is_std_string<T>{};
template<typename T>
constexpr bool is_std_string_v = is_std_string<T>::value;



// only pointer allow polymorphism
template<typename T, typename File, typename R = std::remove_reference_t<T> >
struct has_polymorphism_serialize 
{
    template<
        typename A,
        typename AR = decltype(makeval<A>()->polymorphismSerialize( declval< File*>() )) ,
        typename = std::enable_if_t< std::is_same<AR,bool>::value >,
        typename = std::enable_if_t< std::is_pointer_v<A> >
        >
    constexpr static bool sfinae(int) { return true; }
    template<typename P>
    constexpr static bool sfinae(...) { return false; }
    constexpr static bool value = sfinae<R>(0);
};
template<typename T,typename File>
constexpr bool has_polymorphism_serialize_v = has_polymorphism_serialize<T,File>::value;


template<typename T, typename File, typename R = std::remove_reference_t<T> >
struct has_polymorphism_deserialize 
{
    template<
        typename A,
        typename B = std::remove_pointer_t<A>,
        typename AR = decltype(B::template polymorphismDeserialize<B>( declval< File*>() )) ,
        typename = std::enable_if_t< std::is_pointer_v<AR> >,
        typename = std::enable_if_t< std::is_pointer_v<A> >
        >
    constexpr static bool sfinae(int) { return true; }
    template<typename P>
    constexpr static bool sfinae(...) { return false; }
    constexpr static bool value = sfinae<R>(0);
};
template<typename T,typename File>
constexpr bool has_polymorphism_deserialize_v = has_polymorphism_deserialize<T,File>::value;

// implements serialize / deserialize, call file to do 
template<typename T,typename File, typename R = std::remove_cv_t< std::remove_reference_t<T>>>
struct has_normal_serialize 
{
    template<
        typename P,
        typename AR = decltype(makeval<P>().serialize( declval< File*>() )) ,
        typename = std::enable_if_t< std::is_same<AR,bool>::value >
        >
    constexpr static bool sfinae(int) { return true; }
    template<typename P>
    constexpr static bool sfinae(...) { return false; }
    constexpr static bool value = sfinae<R>(0);
};
template<typename T,typename File>
constexpr bool has_normal_serialize_v = has_normal_serialize<T,File>::value;


template<typename T, typename File, typename R = std::remove_cv_t<T>> 
struct has_normal_deserialize 
{
    template<
        typename P,
        typename AR = decltype(makeval<P>().deserialize( declval< File*>() )) ,
        typename = std::enable_if_t< std::is_same<AR,bool>::value >
        >
    constexpr static bool sfinae(int) { return true; }
    template<typename P>
    constexpr static bool sfinae(...) { return false; }
    constexpr static bool value = sfinae<R>(0);
};                                            
template<typename T,typename File>
constexpr bool has_normal_deserialize_v = has_normal_deserialize<T,File>::value;

template<typename T, typename File>
struct is_serializible: public std::bool_constant< 
        is_serializible_base_type_v<T> || has_normal_serialize_v<T,File> || has_polymorphism_serialize_v<T,File> || is_std_string_v<T> >{};

template<typename T,typename File>
constexpr bool is_serializible_v = is_serializible<T,File>::value;


template<typename T,typename File>
struct is_deserializible: public std::bool_constant< 
        is_serializible_base_type_v<T> || has_normal_deserialize_v<T,File> || has_polymorphism_deserialize_v<T,File> || is_std_string_v<T> >{};

template<typename T,typename File>
constexpr bool is_deserializible_v = is_deserializible<T,File>::value;

enum class EFileOption:uint16{
    None = 0x00,
    AbsolutePath = 0x01,
    ReletivePath = 0x02,
    // read only
    Read = 0x04,
    // write only
    Write = 0x08,
    // read and write same time
    ReadWrite = Read | Write,
    // before each write, offset is position at then end of file, NFS filesystem don't support
    // multi-process write or lead to file corrupt
    Append = 0x10,
    // create file if not exists
    CreateIfNoExists = 0x20,
    // non-block mode
    NonBlock = 0x40,
    Trunc =0x80.
    // 
    Device = 0x100,
};
ENUM_CLASS_FLAGS(EFileOption)

enum class EFileSeek{
    Begin,
    Current,
    End,
    NextContainData,
    NextHole,
};


template<typename T, typename File>
struct DispatchPolymorphSerailize
{
    bool serialize(T* t, File* file) const {
        return t->polymorphismSerialize(file);
    }

    bool deserialize(T*& t, File* file) const {
        //rs_check(t==nullptr);
        t = T::template polymorphismDeserialize<T>(file);
        return t!=nullptr;
    }
};
template<typename T,typename File>
constexpr DispatchPolymorphSerailize<T,File> DispatchPolymorphSerailizeFn{};

enum class FileFixAccess:uint8{
    Read=0,
    Write,
    ReadWrite,
    Num,
};

PROJECT_NAMESPACE_END


#define DEBUG_FILE_SYSTEM 0

#if DEBUG_FILE_SYSTEM

struct GFileSystemDebug {
    inline static bool Enable = false;
    inline static int Tab = 0;
    static constexpr rs::pcchar TabStr[] = { "", "\t", "\t\t", "\t\t\t", "\t\t\t\t", "\t\t\t\t\t", "\t\t\t\t\t\t", "\t\t\t\t\t\t\t", "\t\t\t\t\t\t\t\t" };
    static constexpr int kMaxTabIndex = sizeof(TabStr) / sizeof(rs::pcchar) - 1;
    static rs::pcchar GetTabs() { return TabStr[Tab > kMaxTabIndex ? kMaxTabIndex : Tab]; }
};

struct FileSystemDebugScope {
    FileSystemDebugScope() {
        GFileSystemDebug::Enable = true;
    }
    ~FileSystemDebugScope() {
        GFileSystemDebug::Enable = false;
    }
};
#include"../logger.h"
#include"../../debug_type.hpp"
#include"../common/to_string_protocol.h"
struct ScopeAddTab{
    ScopeAddTab(){ ++GFileSystemDebug::Tab; }
    ~ScopeAddTab(){--GFileSystemDebug::Tab;}
};

#define DATA_TO_STRING(...) "" //rs::toString(__VA_ARGS__).c_str()

#define FILE_SYSTEM_DEBUG_LOG(...) if(GFileSystemDebug::Enable) RS_RELOG( GFileSystemDebug::GetTabs(), __VA_ARGS__); ScopeAddTab PP_CONNECT2(Scope, __LINE__){};
#define FILE_SYSTEM_INLINE RS_FORCENOINLINE
#define CHECK_SERIALIZE_SUCC(Expr)  \
                if(GFileSystemDebug::Enable){ RS_RELOG_LOC( GFileSystemDebug::GetTabs(), " # expr:%s", #Expr);  } \
                succ = Expr; if(!succ){ RS_LOG("expr:%s failed because:%s at file:%s:line:%d",#Expr, file->getError(), __FILE__, __LINE__); }
#else
#define FILE_SYSTEM_DEBUG_LOG(...)
#define FILE_SYSTEM_INLINE RS_FORCEINLINE
#define DATA_TO_STRING(...)
#define CHECK_SERIALIZE_SUCC(Expr) // succ = Expr; if(!succ){ RS_LOG_FILE("expr:%s failed because:%s at file:%s:line:%d",#Expr, file->getError(), __FILE__, __LINE__); }
#endif 



