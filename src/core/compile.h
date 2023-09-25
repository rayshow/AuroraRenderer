/**
 * @file compile.h
 * @author xiongya
 * @brief multi-platform use c++ compile header
 * @version 0.1
 * @date 2022-06-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

//#define RS_COMPILER_DEFINE 0
#define RS_COMPILER_MSVC   1
#define RS_COMPILER_CLANG  2
#define RS_COMPILER_GCC    3
#define RS_COMPILER_INTEL  4

//#define RS_SIMD_DEFINE    0
#define RS_SIMD_SSE       1  //sse or above
#define RS_SIMD_NEON      2  //NEON 
#define RS_SIMD_NONE      3  //FPU

#define RS_SIZE_CHECK     1
#define RS_SIZE_MAX       10000

/*
cpp98: 199711  _MSC_VER always 199711
cpp11: 201103
cpp14: 201402
cpp17: 201703L
C++20: 202002L
*/
#define RS_CPP17       1

#if !defined(__cplusplus) 
#error "not used for c++"
#elif __cplusplus >=201703L 
#define RS_CPP_STD_VER RS_CPP17
#else 
#define RS_CPP_STD_VER RS_CPP11
#error "c++14 and newer should be support"
#endif

#define WSTR( str )   L##str

// OS
// RS_PLATFORM_WINDOW
// RS_PLATFORM_ANDROID
// RS_PLATFORM_LINUX
// RS_PLATFORM_IPHONE
// RS_PLATFORM_MAC
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(	_WIN64)
#	define RS_PLATFORM_WINDOW 1
#	define RS_MAX_PATH        MAX_PATH
#	define RS_ANSI_LINE_TERMINATOR "\r\n"
#	define RS_WIDE_LINE_TERMINATOR L"\r\n"
#elif defined(__ANDROID__)
#	define RS_PLATFORM_ANDROID 1
#	define RS_MAX_PATH  PATH_MAX
#	define RS_ANSI_LINE_TERMINATOR "\n"
#	define RS_WIDE_LINE_TERMINATOR L"\n"
#elif defined(linux) || defined(__linux) || defined(__linux__)
#   define RS_PLATFORM_LINUX   1
#	define RS_MAX_PATH    PATH_MAX
#	define RS_ANSI_LINE_TERMINATOR "\n"
#	define RS_WIDE_LINE_TERMINATOR L"\n"
#elif  defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
 	#include "TargetConditionals.h"
    #if TARGET_OS_IPHONE && TARGET_OS_SIMULATOR
		// iphone simulator
        #define RS_PLATFORM_IPHONE 1
    #elif TARGET_OS_IPHONE && TARGET_OS_MACCATALYST
		// catalyst
        #define RS_PLATFORM_IPHONE 1
    #elif TARGET_OS_IPHONE
        #define RS_PLATFORM_IPHONE 1
    #else
        #define RS_PLATFORM_MAC    1
    #endif

#	define RS_MAX_PATH    1024
#	define RS_ANSI_LINE_TERMINATOR "\r"
#	define RS_WIDE_LINE_TERMINATOR L"\r"
#else
#error "OS not support"
#endif
#if !defined(RS_PLATFORM_WINDOW)
#define RS_PLATFORM_WINDOW 0
#endif
#if !defined(RS_PLATFORM_ANDROID)
#define RS_PLATFORM_ANDROID 0
#endif
#if !defined(RS_PLATFORM_LINUX)
#define RS_PLATFORM_LINUX  0
#endif
#if !defined(RS_PLATFORM_IPHONE)
#define RS_PLATFORM_IPHONE 0
#endif
#if !defined(RS_PLATFORM_MAC)
#define RS_PLATFORM_MAC    0
#endif

// compiler
// RS_COMPILER_MSVC 
// RS_COMPILER_CLANG
// RS_COMPILER_GCC
// RS_COMPILER_INTEL
#if defined(_MSC_VER) && _MSC_VER >=1800
#	if defined(__clang__)
#		define RS_COMPILER_CLANG  1 //clang with msvc
#	else
#		define RS_COMPILER_MSVC   1
#	endif
#elif defined(__clang__) 
#define RS_COMPILER_CLANG         1
#elif defined(__GNUC__) && !defined(__ibmxl__)
#define RS_COMPILER_GCC           1
#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
#define RS_COMPILER_INTEL         1
#else
#error "compiler not support"
#endif
#if !defined(RS_COMPILER_MSVC)
#define RS_COMPILER_MSVC 0
#endif
#if !defined(RS_COMPILER_CLANG)
#define RS_COMPILER_CLANG 0
#endif
#if !defined(RS_COMPILER_GCC)
#define RS_COMPILER_GCC  0
#endif
#if !defined(RS_COMPILER_INTEL)
#define RS_COMPILER_INTEL 0
#endif

#define RS_CACHE_OPT                                           1

//archecture 64
#if defined(__x86_64__) || defined(_M_X64) || defined(_WIN64) || defined(__64BIT__) || defined(_LP64) || defined(__LP64__) \
  || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(__arm64__) || defined(__aarch64__)
#  define RS_ARCH64                                            1             
#else
#  define RS_ARCH64                                            0
#endif

//vector math support
#	if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) //intel or amd
#		define RS_SIMD_SSE  1                                         	 
#   elif  defined(_M_ARM) || defined(_M_ARM64) //mali 
#		define RS_SIMD_NEON 1                          
#	else
#		define RS_SIMD_NONE 1
#	endif
#if !defined(RS_SIMD_SSE)
#define RS_SIMD_SSE 0
#endif
#if !defined(RS_SIMD_NEON)
#define RS_SIMD_NEON  0
#endif
#if !defined(RS_SIMD_NONE)
#define RS_SIMD_NONE 0
#endif

//compile mode. debug or release
#if  defined(DEBUG) || defined(_DEBUG)
#define RS_DEBUG											1
#else 
#define RS_DEBUG                                            0
#endif 

//function like macro
#ifndef rs_likely
//gcc  or clang
#if RS_COMPILER_GCC || RS_COMPILER_CLANG
#define rs_likely(x)    __builtin_expect(!!(x), 1)
#define rs_unlikely(x)  __builtin_expect(!!(x), 0)
#else
#define rs_likely(x)   (x)
#define rs_unlikely(x) (x)
#endif
#endif

#if RS_COMPILER_MSVC
#	define RS_MS_ALIGN(n)                __declspec(align(n))
#	define RS_GCC_ALIGN(n) 
#	define RS_DLLEXPORT                  __declspec(dllexport)            //building as a library
#	define RS_DLLIMPORT	                 __declspec(dllimport)            //building with this library
#	define RS_C_DLL_EXPORT               extern "C" __declspec(dllexport) // export dll/so for c
#	define RS_DEPRECATED(version, msg)   __declspec(deprecated(msg "please update your code"))
#	define RS_CDECL	                     __cdecl			
#   define RS_FASTCALL                   __fastcall
#   define RS_VECTORCALL                 __vectorcall
#	define RS_STDCALL	                 __stdcall										
#	define RS_FORCEINLINE                __forceinline
#	define RS_FORCENOINLINE              __declspec(noinline)
#elif RS_COMPILER_GCC || RS_COMPILER_CLANG //linux or clang
#	define RS_MS_ALIGN(n) 
#	define RS_GCC_ALIGN(n)	              __attribute__((aligned(n)))
#	define RS_DLLEXPORT	                  __attribute__((visibility("default")))
#	define RS_DLLIMPORT 	              __attribute__((visibility("default")))
#	define RS_C_DLL_EXPORT                extern "C"
#	define RS_DEPRECATED(version, msg)	  __attribute__((deprecated(msg "please update your code")))
#	define RS_CDECL	  		
#	define RS_VARARGS    
#	define RS_STDCALL	  
#	define RS_FORCEINLINE inline        __attribute__ ((always_inline))
#	define RS_FORCENOINLINE             __attribute__((noinline))
#   define RS_DISABLE_WARNING(Msg)
#else ///mac
#	define RS_MS_ALIGN(n) 
#	define RS_GCC_ALIGN(n)	             __attribute__((aligned(n)))
#	define RS_DLLEXPORT	
#	define RS_DLLIMPORT
#	define RS_C_DLL_EXPORT     
#	define RS_DEPRECATED(version, msg)   __attribute__((deprecated(MESSAGE "please update your code")))
#	define RS_CDECL	  		
#	define RS_VARARGS    
#	define RS_STDCALL	  
#	if  defined(RS_DEBUG)
#		define RS_FORCEINLINE inline
#	else
#		define RS_FORCEINLINE inline     __attribute__ ((always_inline))
#	endif
#	define RS_FORCENOINLINE              __attribute__((noinline))	
#endif

//lib or dll import export
#define RS_STATIC
#if defined(RS_STATIC)
#	define RS_API 
#else
#	if defined(RS_EXPORT_DLL)
#		define RS_API RS_DLLEXPORT
#   else 
#		define RS_API RS_DLLIMPORT
#	endif
#endif

// app or plugin
#if !defined(RS_PLUGIN)
#define RS_APP 1
#else 
#define RS_APP 0
#endif

#if RS_COMPILER_MSVC
//4514: un-used inline function had been removed
//4710: function had not been inlined
//4505: un-reference local function had been removed
//4996: _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4514 4710 4505 4996)
#endif

// enable memory leak check
#if RS_COMPILER_MSVC && RS_DEBUG
//#define new  new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define rs_open_leak_check()                           \
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); \
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;                  \
	_CrtSetDbgFlag(tmpFlag);                          
#define res_dump_leak() _CrtDumpMemoryLeaks()
#else
#define rs_open_leak_check()
#define res_dump_leak()
#endif


#if RS_DEBUG
	#if RS_COMPILER_MSVC
		#define rs_debugbreak()      __debugbreak()
	#elif defined(__aarch64__)
		#if defined(__APPLE__)
			#define rs_debugbreak() __builtin_debugtrap()
		#else 
			#define rs_debugbreak() __asm__ volatile(".inst 0xd4200000")
		#endif 
	#elif defined(__i386__) || defined(__x86_64__)
		#define rs_debugbreak()     __asm__ volatile("int $0x03")
	#elif defined(__arm__) && !defined(__thumb__)
		#define rs_debugbreak()     __asm__ volatile(".inst 0xe7f001f0")
	#else 
		#include <signal.h>
		#define rs_debugbreak()     raise(SIGTRAP)
	#endif
	#define rs_assert(expr) if(!(expr)){ rs_debugbreak(); }
#else 
	#define rs_debugbreak()	
	#define rs_assert(expr)
#endif // end RS_DEBUG

#define PP_CONNECT2(x,y) PP_CONNECT2_HELPER1(x, y)
#define PP_CONNECT2_HELPER1(x, y ) PP_CONNECT2_HELPER2(x##y)
#define PP_CONNECT2_HELPER2(result ) result

#define RS_STRINGIZE(...) RS_STRINGIZE_FAST(__VA_ARGS__)
#define RS_STRINGIZE_FAST(...) #__VA_ARGS__

#define _AR ar3d
#define RS_NS_BEGIN namespace _AR{ 
#define RS_NS_END }
#define PROJECT_NAMESPACE_BEGIN RS_NS_BEGIN
#define PROJECT_NAMESPACE_END RS_NS_END

#define RS_TFN(T)  template<class...> class T
#define RS_TFN1(T) template<class, class...> class T

// big/little endian
RS_FORCEINLINE bool isLittleEndian()
{
	int t = 1;
	return *((char*)&t) == 1;
}

#define AR_THIS_CLASS(ClassName)  using this_type = ClassName;
#define	AR_ATTRIBUTE(Type, Member)  \
			Type _## Member;\
			RS_FORCEINLINE this_type& set_ ## Member(Type const& Other){ \
				_## Member = Other; \
				return *this; } \
			RS_FORCEINLINE Type const& Member() const{\
				return _## Member; } \
			RS_FORCEINLINE Type& get_ ## Member() { \
					return _## Member; }