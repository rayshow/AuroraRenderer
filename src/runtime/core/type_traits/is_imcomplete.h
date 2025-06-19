#pragma once
#include<type_traits>

// weather a type is complete, a type T* or T& with no T define found is incomplete

template<typename T, typename NoP = std::remove_pointer_t<T> >
struct is_complete 
{
	template<
		typename P,
		typename AR = decltype( void(sizeof(P)) )
		>
	constexpr static bool sfinae(int) { return true; }
	template<typename P>
	constexpr static bool sfinae(...) { return false; }
	constexpr static bool value = sfinae<NoP>(0);
};
template<typename T>
constexpr bool is_complete_v = is_complete<T>::value;