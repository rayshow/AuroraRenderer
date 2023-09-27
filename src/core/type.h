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
#include<array>
#include<unordered_map>
#include<unordered_set>
#include<tuple>
#include<optional>
#include"compile.h" 


PROJECT_NAMESPACE_BEGIN

// base type
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
using achar   = char;
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

template<typename T, typename Allocator= typename std::vector<T>::allocator_type>
using TArray = std::vector<T>;

template<typename T, i32 N>
using TStaticArray = std::array<T, N>;


using String = std::string;
inline static String kEmptyString{""};

template<typename Key, typename Value>
using TPair = std::pair<Key, Value>;

template<typename Key, typename Value>
using TMap = std::unordered_map<Key,Value>;

template<typename Key>
using TSet = std::unordered_set<Key>;

template<typename... Args>
using TTuple = std::tuple<Args...>;

template<typename T>
using TOptional = std::optional<T>;

static_assert(sizeof(u8) == 1, "u8 is not 1 byte.");
static_assert(sizeof(i8) == 1, "i8 is not 1 byte.");
static_assert(sizeof(u16) == 2, "u16 is not 2 byte.");
static_assert(sizeof(i16) == 2, "i16 is not 2 byte.");
static_assert(sizeof(u32) == 4, "u32 is not 4 byte.");
static_assert(sizeof(i32) == 4, "i32 is not 4 byte.");
static_assert(sizeof(u64) == 8, "u64 is not 8 byte.");
static_assert(sizeof(i64) == 8, "i64 is not 8 byte.");

template<typename Int> constexpr Int kIntInvalid = (Int)(-1);

#if defined(max)
#define OLD_MAX max
#undef max
#endif
template<typename Int>
constexpr Int kIntMax = std::numeric_limits<Int>::max();
#if defined(OLD_MAX)
#define max OLD_MAX
#undef OLD_MAX
#endif

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
    template<typename T>
    constexpr explicit TStringView(T&& Other) :Super{ Other } {}
    template< class It, class End >
    constexpr TStringView(It first, End end) : Super{ first, end } {}

    This& emptyStringView() {
        static This sEmptyStringView{};
        return sEmptyStringView;
    }

    This& startsThenRemove(char ch) {
        if (Super::starts_with(ch)) {
            Super::remove_prefix(1);
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
                Super::remove_prefix(i);
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
                Super::remove_suffix(1);
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

    TPair<This, This> splitToTwo(char ch) const
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

    template<typename T>
    This& removeLastBefore(T const& c) {
        SizeType i = Super::rfind(c);
        if (i == this->npos) {
            return emptyStringView();
        }
        Super::remove_suffix(Super::size() - i-1);
        return *this;
    }

};

using StringView = TStringView<char>;
using StringViewPair = TPair<StringView, StringView>;

template<typename Char>
struct RawStrOps;

template<>
struct RawStrOps<char>
{
    static usize length(char const* str) {
        return strlen(str);
    }

    static char* copy(char const* str) {
        return strdup(str);
    }

    static i32 compare(char const* str1, char const* str2, usize n = 0 ) {
        if (n == 0) {
            return strcmp(str1, str2);
        }
        else {
            return strncmp(str1, str2, n);
        }
    }
};


struct MemoryOps
{
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



