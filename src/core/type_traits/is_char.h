#pragma once
#include<type_traits>

template<typename T>
struct is_char:public std::false_type{};
template<> struct is_char<char>: public std::true_type{};
template<> struct is_char<char const>: public std::true_type{};
template<> struct is_char<char volatile>: public std::true_type{};
template<> struct is_char<char const volatile>: public std::true_type{};
template<typename T> constexpr bool is_char_v = is_char<T>::value;