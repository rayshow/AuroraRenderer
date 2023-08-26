/**
 * @file util.h
 * @author xiongya 
 * @brief util macro and type traits
 * @version 0.1
 * @date 2022-06-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once
#include <type_traits>
#include "type.h"


#define RS_PP_UNPACK_CONNECT(x, y)  RS_PP_UNPACK_CONNECT2(x, y)
#define RS_PP_UNPACK_CONNECT2(x, y) RS_PP_UNPACK_CONNECT3(x y)
#define RS_PP_UNPACK_CONNECT3(result) result

#define RS_PP_UNPACK( seq ) RS_PP_UNPACK_HELPER(RS_PP_UNPACK_HELPER, seq)
#define RS_PP_UNPACK_HELPER(...) __VA_ARGS__

#define PP_DEFINE_CONNECT2(x, y) PP_DEFINE_CONNECT2_HELPER(x y)
#define PP_DEFINE_CONNECT2_HELPER(result ) result

#define PP_DEFINE_CONNECT3(x, y) PP_DEFINE_CONNECT3_HELPER1(x ,y)
#define PP_DEFINE_CONNECT3_HELPER1(x, y) PP_DEFINE_CONNECT3_HELPER2(x y)
#define PP_DEFINE_CONNECT3_HELPER2(result ) result


#define RS_PP_REMOVE_BRACK(result) RS_PP_REMOVE_BRACK_HELPER result
#define RS_PP_REMOVE_BRACK_HELPER(...) __VA_ARGS__



template<typename T>
RS_FORCEINLINE void CopyLiteralString(char const* Dest, T SourceChar);

template<int N>
RS_FORCEINLINE void CopyLiteralString(char const* Dest, const char ( & SourceChar)[N] )
{
   memcpy((void*)(const void*)Dest, (void*)(const void*)SourceChar, N);
}

//type traits
template<typename T, T v>
struct compile_t
{
	// type is self
	typedef compile_t<T, v> type;
	// value_type is declare type
	typedef T               value_type;
	// value is declare value
	static constexpr T value = v;
};
template<bool v> using bool_ = compile_t<bool, v>;
using true_ = compile_t<bool, true>;
using false_ = compile_t<bool, false>;
template<typename... Args> struct void_{};
struct null_ {};
template<typename T> struct is_null_ :public false_ {};
template<> struct is_null_< null_> : public true_ {};

namespace __impl
{
	template<typename T, bool IsVoid = std::is_void<T>::value > struct add_rref_helper { typedef T&& type; };
	template<typename T>                              struct add_rref_helper<T, true> { typedef T type; };
    template<typename T, bool isVoid = std::is_void<T>::value > struct add_lref_helper{ typedef T& type; };
	template<typename T>                              struct add_lref_helper<T, true> { typedef T  type; };
    //for some special class 
	template<typename T1, typename T2> struct is_same_impl :false_ {};
}
template<typename T> struct add_rref : public __impl::add_rref_helper<T>{};
template<typename T> using add_rref_t = typename add_rref<T>::type;
template<typename T> struct add_lref : public __impl::add_lref_helper<T>{};
template<typename T> using add_lref_t = typename add_lref<T>::type;
template<typename T> constexpr add_rref_t<T> declval() noexcept;
template<typename T> constexpr add_lref_t<T> makeval() noexcept;


template<bool B, typename T1, typename T2> struct derive_if_c:public T1 {};
template<typename T1, typename T2>         struct derive_if_c<false, T1, T2> :public T2 {};
template<bool B, typename T1, typename T2> 
using derive_if_c_t = typename derive_if_c<B, T1, T2>::type;
template<typename C, typename T1, typename T2>
struct derive_if :public derive_if_c<C::value, T1, T2> {};

template<bool B, typename T> struct _enable_if {  };
template<typename T> struct _enable_if<true,T> { typedef T type; };

template<bool B>
using check = typename _enable_if<B, void>::type;

template<typename T>
using check_t = check< T::value >;

template<typename A,typename B> struct is_same : public __impl::is_same_impl<A,B> {};
template<typename T> struct is_same<T, T> :public true_ {};



#define RS_TT_HAS_MEMBER_DECL_IMPL(TraitName, MemberName, Extra)                        \
namespace __impl{                                                                       \
	template<typename T, typename Ret, RS_PP_REMOVE_BRACK(Extra)  typename ... Args>    \
	struct TraitName##_impl {                                                           \
			template< typename P,                                                       \
			typename AR = decltype(makeval<P>().MemberName(declval<Args>()...)),        \
			typename = check_t< derive_if< is_null_<Ret>, true_, is_same<AR,Ret> > >    \
		>                                                                               \
		constexpr static bool sfinae(int) { return true; }                              \
		template<typename P>                                                            \
		constexpr static bool sfinae(...) { return false; }                             \
		constexpr static bool value = sfinae<T>(0);                                     \
	};                                                                                  \
}

// define a type trait to test weather anyRet(template) T.MemberName(args...) is well-formed
#define RS_TT_HAS_MEMBER_DECL_EXTRA( TraitName, MemberName, Extra)                      \
		RS_TT_HAS_MEMBER_DECL_IMPL( TraitName, MemberName, (Extra))                     \
		template<typename T, typename Ret, typename... Args>                            \
		static constexpr bool TraitName##_v                                             \
			= __impl::TraitName##_impl<T, Ret, Args...>::value;                         \
		template<typename T, typename Ret, typename... Args>                            \
		struct TraitName :bool_< TraitName##_v<T,Ret,Args...> >                         \
		{};
        
#define RS_TT_HAS_MEMBER_WITH_RET_DECL(TraitName, MemberName, Ret) RS_TT_HAS_MEMBER_WITH_RET_DECL_EXTRA(TraitName, MemberName, Ret, )
#define RS_TT_HAS_MEMBER_DECL( TraitName, MemberName) RS_TT_HAS_MEMBER_DECL_EXTRA( TraitName, MemberName, )

#define SAFE_STRUCT_STRING(Obj, Name) ((Obj) && (Obj->Name)) ? (Obj->Name) : "(None)"


RS_FORCEINLINE bool StringContain(const char* str1, const char* str2){
	return strstr(str1,str2) != nullptr;
}

