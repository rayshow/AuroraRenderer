#pragma once
#include<type_traits>
#include"is_char.h"

/* char* c */
template<typename T>
struct is_raw_string:public std::bool_constant< std::is_pointer_v<T> && is_char_v< std::remove_pointer_t<T> > >{};
template<typename T>
constexpr bool is_raw_string_v = is_raw_string<T>::value;


