#pragma once

#include<type_traits>
#include<string>
#include<vector>
#include"core/compile.h"

PROJECT_NAMESPACE_BEGIN

/*
* has 
struct A
{
  std::string toString(){...}
};
*/
template<typename T, typename R = std::remove_reference_t<T> >
struct has_to_string_member
{
    template<
        typename A,
        typename AR = decltype(makeval<A>().toString()) ,
        typename = std::enable_if_t< std::is_same_v<AR,std::string> >
        >
    constexpr static bool sfinae(int) { return true; }
    template<typename P>
    constexpr static bool sfinae(...) { return false; }
    constexpr static bool value = sfinae<R>(0);
};
template<typename T>
constexpr bool has_to_string_member_v = has_to_string_member<T>::value;

/* has std::to_string */
template<typename T, typename R = std::remove_all_extents_t< std::remove_reference_t< std::remove_cv_t<T>>>>
struct has_std_to_string:public std::bool_constant< std::is_integral_v<R> || std::is_floating_point_v<R> || std::is_enum_v<R> > {};
template<typename T>
constexpr bool has_std_to_string_v = has_std_to_string<T>::value;


template<typename T>
struct has_to_string: public std::bool_constant< has_to_string_member_v<T> || has_std_to_string_v<T> || std::is_pointer_v<T> >{};
template<typename T>
constexpr bool has_to_string_v = has_to_string<T>::value;


#define AR_ENABLE_IF_HAS_TO_STRING(Type) typename R = std::remove_reference_t<Type>, typename = std::enable_if_t< has_to_string_v<R> >
#define AR_ENABLE_IF_HAS_TO_STRING_SINGLE_ITEM(Type) typename R = std::remove_reference_t<Type>, typename = std::enable_if_t< has_to_string_v<R> && !std::is_array_v<R> && !is_std_string_v<T> >


template<typename T, typename... Args>
struct Wrap{};

inline const std::string kDotString{", "};
inline const std::string kNullptrString{"nullptr"};
inline const std::string kQuoteString{"\""};

struct ToStringProtocol
{
    template<typename T,  AR_ENABLE_IF_HAS_TO_STRING_SINGLE_ITEM(T) >
    static std::string toString(T&& t)
    {
        static std::string SEmpty{};
        if constexpr(std::is_enum_v<R>){
            // enum get the integer
            return toString( (int)t );
        }
        else if constexpr(is_char_v<R>){
            return std::string{t};
        }
        else if constexpr(has_std_to_string_v<R>){
            return std::to_string(std::forward<T>(t));
        }
        else if constexpr(std::is_pointer_v<R>){
            if(t==nullptr){
                return kNullptrString;
            }
            if constexpr( has_to_string_v< std::remove_pointer_t<R>> ){
                return toString(*t);
            }else{
                char buffer[128]{0};
                snprintf(buffer, 128, "%p",  (void*)t);
                return std::string{buffer};
            }
        }
        else if constexpr(has_to_string_member_v<R>){
            static std::string BracketBegin{"{"};
            static std::string BracketEnd{"}"};
            std::string&& content = t.toString();
            if(content.empty()){
                return content;
            }
            return  BracketBegin+ content + BracketEnd;
        }
        return SEmpty;
    }


    static constexpr u32 kLengthDisplay = 16;
    template<typename T, typename R = std::remove_reference_t<T>>
    static std::string toString(T const* array, u64 length)
    {
        static std::string SArrayBegin{"[ "};
        static std::string SArrayEnd{" ]"};
        static std::string SEmpty{"[]"};
        if(array==nullptr || length == 0){
            return SEmpty;
        }

        u32 showLength = length;
        if(length>kLengthDisplay){
            showLength=kLengthDisplay;
        }
        
        if constexpr(is_char_v<T>){
            return std::string{array};
        }else if constexpr(is_raw_string_v<T> || has_to_string_v<R>){
            std::string content{ SArrayBegin };
            for(u64 i=0; i<showLength;++i){
                if(i==0){
                    content += toString(array[i]);
                }else{
                    content += kDotString + toString(array[i]);
                }
                if(i>kLengthDisplay){
                    content += kDotString + "...";
                    break;
                }
            }
            
            content +=SArrayEnd;
            return content;
        }else{
            return toString((void const*)array) +", "+ toString(length);
        }
        
        return SEmpty;
    }

    template<typename T, u32 N>
    static std::string toString(T const (&array)[N]){
        return toString(array, N);
    }

    template<typename T, typename Allocator>
    static std::string toString(std::vector<T, Allocator> const& vector){
        return toString(vector.data(), vector.size());
    }

    // uniform
    static std::string toString(std::string const& str){
        return kQuoteString+str+kQuoteString;
    }

    static std::string toString(char const* rawStr){
        if(rawStr == nullptr){
            return kNullptrString;
        }
        return kQuoteString+std::string{rawStr}+kQuoteString;
    }

    template<typename T>
    std::string toString2(T&& t){
        return toString(t);
    }
    template<typename T, typename... Args, typename = std::enable_if_t< (sizeof...(Args)>0) >>
    std::string toString2(T&& t, Args&&... args){
        return toString(std::forward<T>(t) ) + kDotString + toString2(std::forward<Args>(args)...);
    }
};

#define AR_BEGIN_TO_STRING(ClassName) std::string ret{"class="}; ret.reserve(128); ret+=#ClassName
#define AR_BEGIN_DERIVED_TO_STRING(Drived, Super) AR_BEGIN_TO_STRING(Drived); ret+=std::string{", super={"}+Super::toString()+"}";
#define AR_TO_STRING(Name)  ret += kDotString + #Name + " = " + ToStringProtocol::toString(Name);
#define AR_TO_STRING_WITH_NAME(Name,Value)  ret += kDotString + #Name + " = " + ToStringProtocol::toString(Value);
#define AR_ARRAY_TO_STRING(Name, Length)  ret += kDotString + #Name + " = " + ToStringProtocol::toString(Name, Length);
#define AR_ARRAY_TO_STRING_WITH_NAME(Name, Value, Length)  ret += kDotString + #Name + " = " + ToStringProtocol::toString(Value, Length);
#define AR_END_TO_STRING() return ret;

#define GET_OBJECT_RAW_STRING(Obj) rs::ToStringProtocol::toString(Obj).c_str()
#define GET_OBJECT_ARRAY_RAW_STRING(Obj, Len) rs::ToStringProtocol::toString(Obj, Len).c_str()
#define GET_OBJECT_STRING(Obj) rs::ToStringProtocol::toString(Obj)

#undef AR_ENABLE_IF_HAS_TO_STRING

PROJECT_NAMESPACE_END
