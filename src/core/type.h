/**
 * @file type.h
 * @author xiongya
 * @brief uniform type define
 * @version 0.1
 * @date 2022-06-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include<new>       /* nullptr_t and nothrow_t */
#include<cctype>
#include<limits>
#include<string>
#include<string_view>
#include<vector>
#include<algorithm>
#include<array>
#include <cstdarg>
#include<unordered_map>
#include<unordered_set>
#include<tuple>
#include<optional>
#include<wchar.h>
#include<stdlib.h>
#include<memory>
#include<functional>
#include<type_traits>
#include"compile.h" 


// avoid system header #define min/max
#if defined(max)
#define system_max max
#undef max
#endif
#if defined(min)
#define system_min min
#undef min
#endif


PROJECT_NAMESPACE_BEGIN
//////////////////////////////////////////////////
///
/// base type
using u8  =  unsigned char;
using u16 =  unsigned short int;
using u32 =  unsigned int;
using u64 =  unsigned long long;
using i8  =  signed char;
using i16 =  signed short int;
using i32 =  signed int;
using i64 =  signed long long;
using f64 =  long double;
using f32 =  float;
using wchar   = wchar_t;
using char16  = char16_t;
using char32  = char32_t;

template<i32 size> struct size_traits { static_assert(size != 4 || size != 8, "unkown ptr size."); };
template<>         struct size_traits<4> { using size_t = u32 ; using diff_t = i32; };
template<>         struct size_traits<8> { using size_t = u64 ; using diff_t = i64; };
using usize = typename size_traits<sizeof(void*)>::size_t;
using isize = typename size_traits<sizeof(void*)>::diff_t;
using uintptr = usize;
using intptr = isize;
using nullptr_t = decltype(nullptr);
using nothrow_t = std::nothrow_t;
constexpr nothrow_t nothrow;

static_assert(sizeof(u8) == 1, "u8 is not 1 byte.");
static_assert(sizeof(i8) == 1, "i8 is not 1 byte.");
static_assert(sizeof(u16) == 2, "u16 is not 2 byte.");
static_assert(sizeof(i16) == 2, "i16 is not 2 byte.");
static_assert(sizeof(u32) == 4, "u32 is not 4 byte.");
static_assert(sizeof(i32) == 4, "i32 is not 4 byte.");
static_assert(sizeof(u64) == 8, "u64 is not 8 byte.");
static_assert(sizeof(i64) == 8, "i64 is not 8 byte.");

template<typename Int> constexpr Int kInvalidInteger = (Int)(-1);

template<typename Int>
constexpr Int kIntMax = std::numeric_limits<Int>::max();

/////////////////////////////////////////////////////
/// some basic struct
template<typename T, i32 N>
using TStaticArray = std::array<T, N>;

template<typename Key, typename Value>
using TPair = std::pair<Key, Value>;

template<typename... Args>
using TTuple = std::tuple<Args...>;

template<typename T>
using TOptional = std::optional<T>;

template<typename T>
using TUniquePtr = std::unique_ptr<T>;

template<typename T>
using TFunction = std::function<T>;

template<typename T>
struct TVector2
{
    AR_THIS_CLASS(TVector2);
    AR_ATTRIBUTE(T, x);
    AR_ATTRIBUTE(T, y);

    TVector2() = default;
    TVector2(T const& inX, T const& inY):_x{inX}, _y{inY} {}
};
using Vector2 = TVector2<f32>;
using I32Vector2 = TVector2<i32>;

template<typename T>
struct TRect
{
    AR_THIS_CLASS(TRect)
    AR_ATTRIBUTE(T, x);
    AR_ATTRIBUTE(T, y);
    AR_ATTRIBUTE(T, width);
    AR_ATTRIBUTE(T, height);
};
using I32Rect = TRect<i32>;
using f32Rect = TRect<f32>;

template<typename T>
struct TRange
{
    T begin{};
    T length{};
    
    T end(){ return begin + length; }
    bool isValid(){ return length > 0; }
};
using Range     = TRange<i32>;
using SizeRange = TRange<usize>;

struct ReadableTime
{
    u32 year;
    u32 month;
    u32 day;
    u32 hour;
    u32 minute;
    u32 second;
    u32 millisecond;
};


///////////////////////////////////////////////////////////////
/// string type

// is char
template<typename T>
struct is_char :public std::false_type {};
template<> struct is_char<char> : public std::true_type {};
template<> struct is_char<char const> : public std::true_type {};
template<> struct is_char<char volatile> : public std::true_type {};
template<> struct is_char<char const volatile> : public std::true_type {};
template<typename T> constexpr bool is_char_v = is_char<T>::value;


// is char*
template<typename T>
struct is_raw_string :public std::bool_constant< std::is_pointer_v<T> && is_char_v< std::remove_pointer_t<T> > > {};
template<typename T>
constexpr bool is_raw_string_v = is_raw_string<T>::value;

enum class EStringCmp: i8
{
    Less = -1,
    Equal =0,
    Greater =1,
};

static EStringCmp cmpResult(i32 result)
{
    if(result == 0) {
        return EStringCmp::Equal;
    }else if(result <0 )
    {
        return EStringCmp::Less;
    }else
    {
        return EStringCmp::Greater;
    }
}

template<typename Char> struct RawStrOps;

template<>
struct RawStrOps<char>
{
    static char const* empty(){
        return "";
    }
    
    static usize length(char const* str)
    {
        return strlen(str);
    }
    
    static usize length(char const* str, usize length) {
        return strnlen_s(str, length);
    }

    static char* copy(char const* str) {
        return _strdup(str);
    }
    
    static EStringCmp compare(char const* str1, char const* str2){
        return cmpResult(strcmp(str1, str2));
    }

    static EStringCmp compare(char const* str1, char const* str2, usize n) {
        return cmpResult(strncmp(str1, str2, n));
    }

    static void format(char* buffer, usize length, char const* format, va_list vargs) {
        _vsnprintf_s( buffer, length, length, format, vargs);
    }
};

template<>
struct RawStrOps<wchar>
{
    static wchar const* empty(){
        return L"";
    } 
    
    static usize length(wchar const* str)
    {
        return wcslen(str);
    }
        
    static usize length(wchar const* str, isize length) {
        return wcsnlen_s(str, length);
    }

    static wchar const* findLast(wchar const* wstr, wchar ch)
    {
        return wcsrchr(wstr, ch);
    }

    static wchar* copy(wchar const* str) {
        return _wcsdup(str);
    }

    static EStringCmp compare(wchar const* str1, wchar const* str2)
    {
        return cmpResult(wcscmp(str1, str2));
    }

    static EStringCmp compare(wchar const* str1, wchar const* str2, isize n = 0) {
        return cmpResult( wcsncmp(str1, str2, n));
    }

    static void format(wchar* buffer, usize length,  wchar const* format, va_list vargs) {
        _vsnwprintf_s( buffer, length, length, format, vargs);
    }
};

template<typename From, typename To>
struct RawStrConvert {};

template<>
struct RawStrConvert<wchar_t, char>
{
    static usize toBuffer(wchar_t const* source, char* buf, usize buflen) {
        return wcstombs(buf, source, buflen);
    }

    static TUniquePtr<char[]> to(wchar const* source)
    {
        usize length = RawStrOps<wchar>::length(source) * 2;
        TUniquePtr<char[]> buf = std::make_unique<char[]>(length + 1);
        if (nullptr == buf) return nullptr;
        buf[length] = 0;
        toBuffer(source, buf.get(), length);
        return buf;
    }
};

template<>
struct RawStrConvert<char, wchar_t>
{
    static isize convertTo(char const* source, wchar_t* buf, isize buflen) {
        return mbstowcs(buf, source, buflen);
    }
};

template<typename Char> struct TStringView;

template<typename Char, typename Allocator = typename std::basic_string<Char>::allocator_type >
struct TString: public std::basic_string<Char>
{
    using Super = std::basic_string<Char>;
    using ViewType = TStringView<Char>;
    using ThisType = TString;
    using CharType = Char;
    using AllocatorType = Allocator;
    using TraitsType = typename std::basic_string<Char>::traits_type;
public:
 
    using Super::operator+=;
    using Super::operator[];
    using Super::at;
    using Super::front;
    using Super::back;
    using Super::begin;
    using Super::end;
    using Super::rbegin;
    using Super::rend;
    using Super::crbegin;
    using Super::crend;
    using Super::find_last_of;
    using Super::substr;

    TString(): Super() {}
    TString(Char const* rawStr): Super(rawStr) {}
    TString(Char const* rawStr, usize length): Super(rawStr, length) {}
    TString(Char const* rawStr, usize pos, usize length): Super(rawStr, pos, length) {}
    TString(Super const& str): Super(str) {}
    TString(Super&& str): Super(std::move(str)) {}
    TString(usize length, Char ch ): Super(length, ch) {}
    
    static TString& emptyString()
    {
        static TString sEmptyString{ RawStrOps<Char>::empty() };
        return sEmptyString;
    }
    
    bool  isEmpty() const{ return Super::empty(); }
    usize findLastOf(Char ch, usize pos = Super::npos) const{ return Super::find_last_of(ch, pos); }
    usize findLastOf(Char const* rawStr, usize pos = Super::npos) const{ return Super::find_last_of(rawStr, pos); }
    usize findLastOf(Super const& str, usize pos = Super::npos) const{ return Super::find_last_of(str); }
    usize findLastOf(TStringView<Char> const& view, usize pos = Super::npos) const;
    
    
    ThisType& substr(usize pos, usize length) {
        return ThisType{*this, pos, length};
    }

    void localFormat(Char const* format, ...) {
        va_list vargs;
        va_start(vargs, format);
        RawStrOps<Char>::format( Super::data(), Super::length(), format, vargs);
        va_end(vargs);
    }

    template<typename ... Args>
    static TString format(usize reserveLength, Char const* format, Args&& ...args)
    {
        TString string{reserveLength, 0};
        string.localFormat(format, std::forward<Args>(args)...);
        return string;
    }

    // delay implements
    operator ViewType();
    TString(TStringView<Char> const& View);
};

using String = TString<char>;
using WString = TString<wchar>;
template<typename T>    struct is_string :public std::false_type {};
template<typename Char> struct is_string< TString<Char> >: std::true_type {};
#define ArCheckType(T, Validator) typename = std::enable_if_t<Validator<T>::value>

template<typename Char>
struct TStringView : public std::basic_string_view<Char>
{
    using Super = std::basic_string_view<Char>;
    using SizeType = typename Super::size_type;
    using This = TStringView;

    constexpr TStringView() noexcept :Super{} {}
    constexpr TStringView(TStringView const& Other) noexcept :Super{ Other } {}
    constexpr TStringView(Char const* str) :Super{ str } {}
    constexpr TStringView(Char const* str, size_t count) : Super{ str, count } {}
    template<typename T, ArCheckType(T, is_string)>
    constexpr TStringView(T const& Other) :Super{ Other } {}
    template< class It, class End >
    constexpr TStringView(It first, End end) : Super{ first, end } {}

    using Super::remove_prefix;
    using Super::remove_suffix;
    using Super::starts_with;

    // redirect
    void removePrefix(usize n) { Super::remove_prefix(n); }
    void removeSuffix(usize n) { Super::remove_suffix(n); }
    bool startWiths(Char ch){ return Super::starts_with(ch); }
    

    This& emptyStringView() {
        static This sEmptyStringView{RawStrOps<Char>::empty()};
        return sEmptyStringView;
    }

    This& startsThenRemove(char ch) {
        if (startWiths(ch)) {
            removePrefix(1);
        }
        return emptyStringView();
    }

    This startsThenRemove(char ch) const {
        if (Super::starts_with(ch)) {
            return This{ Super::data(), Super::size() - 1 };
        }
        return This{};
    }

    This& stripLeft() {
        for (SizeType i = 0; i < Super::size(); ++i) {
            if (!isspace(Super::at(i))) {
                removePrefix(i);
                break;
            }
        }
        return *this;
    }

    This stripLeft() const {
        This copy(*this);
        return copy.stripLeft();
    }

    This& stripRight() {
        for (auto c : *this) {
            if (isspace(c)) {
                removeSuffix(1);
            }
        }
        return *this;
    }
    This stripRight() const {
        This copy(*this);
        return copy.stripRight();
    }

    This& strip() {
        return stripLeft().stripRight();
    }

    This strip() const {
        This copy(*this);
        return copy.strip();
    }

    TPair<This, This> splitToTwo(Char ch) const
    {
        SizeType i = 0;
        Char const* data = Super::data();
        for (; i < Super::size(); ++i) {
            if (data[i] == ch) {
                break;
            }
        }
        if (i == Super::size()) {
            return TPair{ *this, TStringView{} };
        }
        return TPair{ TStringView{ data,i}, TStringView{ data + i + 1, Super::size() - i - 1 } };
    }
    
    This& removeAfter(Char c) {
        SizeType i = Super::rfind(c);
        if (i == this->npos) {
            return emptyStringView();
        }
        removeSuffix(Super::size() - i-1);
        return *this;
    }

    void localFormat(Char const* format, ...)
    {
        va_list vargs;
        va_start(vargs, format);
        RawStrOps<Char>::format(Super::data(), Super::length(), vargs, format);
        va_end(vargs);
    }
};



// delay
template<typename Char, typename Allocator>
TString<Char, Allocator>::operator TStringView<Char>()
{
    return TStringView<Char>{ Super::data(), Super::length()  };
}

template<typename Char, typename Allocator>
usize TString<Char, Allocator>::findLastOf(TStringView<Char> const& view, usize pos ) const
{
    return Super::find_last_of(view, pos);
}

template <typename Char, typename Allocator>
TString<Char, Allocator>::TString(TStringView<Char> const& View) : Super{View.data(), View.length()} {}

template <typename Char, typename Allocator, typename T, typename Super = typename TString<Char, Allocator>::Super >
Super operator+(TString<Char, Allocator>& str, T const& t)
{
    return static_cast< Super const& >(str) + t;
}


using StringView = TStringView<char>;
using StringViewPair = TPair<StringView, StringView>;
using WStringView = TStringView<wchar>;
template<typename T>    struct is_string_view :public std::false_type {};
template<typename Char> struct is_string_view< TStringView<Char> >: std::true_type {};


struct TStringStream
{
private:
    String str;
    usize  pos;
public:
    using CharType = typename String::value_type;

    void addSpace(usize length)
    {
        str.append(length, 0);
    }

    void add(CharType ch)
    {
        str[pos++] = ch;
    }
};

class Path : public WString
{
private:
    bool _bAbsolution{false};
    
public:
    Path operator/(Path const& path)
    {
        Path copy{*this};
        copy += path;
        return copy;
    }

    
};

////////////////////////////////////////////////////////
/// general container: add some handy function


template<typename Key, typename Value>
using TMap = std::unordered_map<Key,Value>;

template<typename Key>
using TSet = std::unordered_set<Key>;


template<typename T, typename Allocator= typename std::vector<T>::allocator_type>
class TArray: public std::vector<T, Allocator>
{
public:
    using Super = std::vector<T, Allocator>;
    using ThisType = TArray<T, Allocator>;
    using ElementType = T;

    
    using Super::operator[];
    using Super::data;
    using Super::at;
    using Super::front;
    using Super::back;
    
    using Super::begin;
    using Super::end;
    using Super::rbegin;
    using Super::rend;
    using Super::crbegin;
    using Super::crend;
    using Super::cbegin;
    using Super::cend;
    
    using Super::clear;
    using Super::size;
    using Super::empty;
    using Super::reserve;
    using Super::resize;
    using Super::assign;
    using Super::capacity;
    using Super::push_back;
    using Super::pop_back;
    using Super::swap;
    
    usize length() const{ return Super::size(); }
    void pushBack(T const& elem){ Super::push_back(elem); }
    void popBack(){ Super::pop_back(); }
};

template<typename T>
struct TArrayView
{
public:
    using ThisType = TArrayView<T>;

    template<typename Allocator>
    TArrayView(TArray<T, Allocator> const& inArray)
        : _dataRef{ static_cast<T*>(inArray.data()) }
        , _length{ inArray.length() }
    {
    }

    TArrayView(T const* inRawArray, usize inSize)
        : _dataRef{ static_cast<T*>(inRawArray) }
        , _length{ _length }
    {}

    TArrayView(TArrayView const& inOther)
        : _dataRef{ const_cast<T*>(inOther._dataRef) }
        , _length{ inOther._length }
    {}

    TArrayView(TArrayView&& inOther)
        : _dataRef{ const_cast<T*>(inOther._dataRef) }
        , _length{ inOther._length }
    {
        inOther.reset();
    }

    TArrayView(TArrayView const& inOther, usize inLength)
        : _dataRef{ const_cast<T*>(inOther._dataRef) }
        , _length{ std::min(inOther._length, inLength) }
    {}


    template<i32 N>
    TArrayView(T const (&fixArray)[N])
        : _dataRef{ const_cast<T*>(static_cast<T const*>(fixArray)) }
        , _length{N}
    {}
    
    TArrayView& operator=(TArrayView const& other) {
        if (std::addressof(other) != this) {
            _dataRef = other._dataRef;
            _length = other._length;
        }
    }

    TArrayView& operator=(TArrayView&& other) {
        if (std::addressof(other) != this) {
            _dataRef = other._dataRef;
            _length = other._length;
            other.reset();
        }
    }

    template<i32 N>
    TArrayView& operator=(T const (&fixArray)[N]) {
        _dataRef = const_cast<T*>(fixArray);
        _length = N;
    }

    T& operator[](usize index) {
        ARAssert(index < _length);
        return _dataRef[index];
    }

    T const& operator[](usize index) const {
        ARAssert(index < _length);
        return _dataRef[index];
    }

    bool isValid() const {
        return _dataRef != nullptr && _length > 0;
    }

    void reset() {
        _dataRef = nullptr;
        _length = 0;
    }

    usize length() const {
        return _length;
    }

private:
    T* _dataRef;
    usize _length;
};


namespace AlgOps {

    struct RawStrEqual {
        template<typename T>
        constexpr bool operator()(T const* first, T const* second) {
            return 0 == RawStrOps< T>::compare(first,second);
        }
    };
    struct RawStrLess {
        template<typename T>
        constexpr bool operator()(T const* first, T const* second) {
            return RawStrOps<T>::compare(first, second) < 0;
        }
    };

    constexpr RawStrEqual rawStrEqual{};
    constexpr RawStrLess  rawStrLess{};
    constexpr std::equal_to<> defaultEqualTo{};
    constexpr std::less<>     defaultLess{};

    template<typename Container, typename SortFn, typename IdentityFn>
    AR_FORCEINLINE void unique(Container& container, SortFn&& sortFn, IdentityFn identityFn )
    {
        std::sort(container.begin(), container.end(), std::forward<SortFn>(sortFn) );
        auto&& last = std::unique(container.begin(), container.end(), std::forward<IdentityFn>(identityFn) );
        container.erase(last, container.end());
    }

    template<typename Container, typename SortFn>
    AR_FORCEINLINE void unique(Container& container, SortFn&& sortFn)
    {
        unique(container, sortFn, defaultEqualTo);
    }

    template<typename Container>
    AR_FORCEINLINE void unique(Container& container)
    {
        unique(container, defaultLess, defaultEqualTo);
    }

    template<typename It,typename Pred>
    AR_FORCEINLINE void sort(It const& begin, It const& end, Pred&& pred) {
        std::sort(begin, end, std::move(pred));
    }

    template<typename It, typename Pred>
    AR_FORCEINLINE void sort(It const& begin, It const& end) {
        std::sort(begin, end);
    }

    template<typename T>
    AR_FORCEINLINE T const& min2(T const& first, T const& second) {
        return std::min(first, second);
    }

    template<typename T>
    AR_FORCEINLINE T const& max2(T const& first, T const& second) {
        return std::max(first, second);
    }

    constexpr double DIVKB = 1.0 / 1024.0;
    constexpr double DIVMB = 1.0 / (1024.0 * 1024.0);
    constexpr double DIVGB = 1.0 / (1024.0 * 1024.0 * 1024.0);
    constexpr double DIVTB = 1.0 / (1024.0 * 1024.0 * 1024.0 * 1024.0);
    constexpr double KB(double bytes) {
        return bytes * DIVKB;
    }

    constexpr double MB(double bytes) {
        return bytes * DIVMB;
    }

    constexpr double GB(double bytes) {
        return bytes * DIVGB;
    }
};

namespace MemoryOps
{
    template<typename T>
    AR_FORCEINLINE void zero(T& t) {
        memset(&t, 0, sizeof(T));
    }

    template<typename T>
    AR_FORCEINLINE void clearToByte(T& t, u8 value) {
        memset(&t, value, sizeof(T));
    }

    AR_FORCEINLINE i32 compare(void* source, void* dest, usize length)
    {
        return memcmp(source, dest, length);
    }

    template<typename T, usize Len>
    AR_FORCEINLINE i32 compare(T const (&source)[Len], T const (& dest)[Len])
    {
        return memcmp(source, dest, sizeof(T)*Len);
    }

    template<typename R>
    AR_FORCEINLINE R* copyArray(R const* array, usize length = 1) {
        R* newArray = nullptr;
        usize size = 0;
        if (array && length > 0) {
            if constexpr (std::is_void_v<R>) {
                size = length;
            }
            else {
                size = sizeof(R) * length;
            }
            newArray = reinterpret_cast<R*>(malloc(size));
            ARAssert(newArray != nullptr);
            if constexpr (is_raw_string_v<R>) {
                // raw-string array
                for (u32 i = 0; i < length; ++i) {
                    newArray[i] = array[i] ? strdup(array[i]) : nullptr;
                }
            }
            else if constexpr (std::is_pod_v<R> || std::is_void_v<R>) {
                // raw bytes or pod array
                memcpy(newArray, array, size);
            }
            // need constructor
            else {
                for (usize i = 0; i < length; ++i) {
                    // copy constructor
                    new(newArray + i) R{ array[i] };
                }
            }
        }
        return newArray;
    }

    template<typename R, typename RawR = std::remove_const_t<R> >
    AR_FORCEINLINE R* newDefaultArray(usize length = 1) {
        RawR* newArray = nullptr;
        if (length > 0) {
            usize size = 0;
            if constexpr (std::is_void_v<R>) {
                size = length;
            }
            else {
                size = sizeof(R) * length;
            }
            newArray = reinterpret_cast<RawR*>(malloc(size));
            if (!newArray) { ARAssert(false); return nullptr; }
            if constexpr (std::is_pod_v<R> || std::is_void_v<R>) {
                memset(newArray, 0, size);
            }
            else {
                // not pod and not void
                for (usize i = 0; i < length; ++i) {
                    // default constructor
                    new(newArray + i) R{};
                }
            }
        }
        return newArray;
    }

    template<typename R>
    AR_FORCEINLINE R* copy(R const* object) {
        if constexpr (is_char_v<R>) {
            // raw-string
            return object ? strdup(object) : nullptr;
        }
        else {
            return safe_new_copy_array(object, 1);
        }
    }

    template<typename T, typename NoConstT = std::remove_const_t<T>>
    void safeDelete(T*& t) {
        NoConstT* addr = const_cast<NoConstT*>(t);

        if constexpr (std::is_void_v<T> || std::is_pod_v<T> || is_char_v<T>) {
            free(addr);
            t = nullptr;
        }
        else {
            sizeof(*t);
            if (t) { t->~NoConstT(); free(t);  t = nullptr; }
        }
    }
    template<typename T >
    void safeDeleteArray(T*& t, u32 length) {
        if (!t || length == 0) return;
        if constexpr (is_raw_string_v<T>) {
            ARAssert(length > 0);
            using NoConstPointerT = std::remove_const_t< std::remove_pointer_t<T>>;
            for (u32 i = 0; i < length; ++i) {
                NoConstPointerT* addr = const_cast<NoConstPointerT*>(t[i]);
                free(addr);
                addr = nullptr;
            }
            free(const_cast<NoConstPointerT**>(const_cast<NoConstPointerT* const*>(t)));
            t = nullptr;
        }
        else if constexpr (std::is_void_v<T> || std::is_pod_v<T>) {
            using NoConstT = std::remove_const_t< T>;
            NoConstT* addr = const_cast<NoConstT*>(t);
            free(addr);
            t = nullptr;
        }
        else {
            sizeof(*t);
            //using NoConstT = std::remove_const_t< T>;
            for (u32 i = 0; i < length; ++i) {
                t[i].~T();
            }
            free(t);
            t = nullptr;
        }
    }
};

PROJECT_NAMESPACE_END

template<typename Char>
struct std::hash< ar3d::TString<Char> >
{
    std::size_t operator()(ar3d::TString<Char> const& str) const
    {
        return std::hash< typename ar3d::TString<Char>::Super>{}(str);
    }
};

template<typename Char>
struct std::hash< ar3d::TStringView<Char> >
{
    std::size_t operator()(ar3d::TString<Char> const& str) const
    {
        return std::hash< typename ar3d::TStringView<Char>::Super>{}(str);
    }
};