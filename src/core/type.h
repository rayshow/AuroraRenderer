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
#include"compile.h" 

RS_NS_BEGIN

// base type
using u8  =  unsigned char;
using u16 =  unsigned short int;
using u32 =  unsigned int;
using u64 =  unsigned long long;
using i8  =  signed char;
using i16 =  signed short int;
using i32 =  signed int;
using i64 =  signed long long;
using ldouble = long double;
using wchar   = wchar_t;
using achar   = char;
using char16  = char16_t;
using char32  = char32_t;
using rawstr  = char*;
using crawstr = char const*;
template<i32 size> struct size_traits { static_assert(size != 4 || size != 8, "unkown ptr size."); };
template<>         struct size_traits<4> { using size_t = u32 ; using diff_t = i32; };
template<>         struct size_traits<8> { using size_t = u64 ; using diff_t = i64; };
using size_t = typename size_traits<sizeof(void*)>::size_t;
using diff_t = typename size_traits<sizeof(void*)>::diff_t;
using uintptr = size_t;
using intptr = diff_t;

//define nullptr_t 
using nullptr_t = decltype(nullptr);

//define nothrow_t
using nothrow_t = std::nothrow_t ;
constexpr nothrow_t nothrow;

static_assert(sizeof(u8) == 1, "u8 is not 1 byte.");
static_assert(sizeof(i8) == 1, "i8 is not 1 byte.");
static_assert(sizeof(u16) == 2, "u16 is not 2 byte.");
static_assert(sizeof(i16) == 2, "i16 is not 2 byte.");
static_assert(sizeof(u32) == 4, "u32 is not 4 byte.");
static_assert(sizeof(i32) == 4, "i32 is not 4 byte.");
static_assert(sizeof(u64) == 8, "u64 is not 8 byte.");
static_assert(sizeof(i64) == 8, "i64 is not 8 byte.");

template<typename T> constexpr T kInvalidInteger = (T)(-1);

RS_NS_END



