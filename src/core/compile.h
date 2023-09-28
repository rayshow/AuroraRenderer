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

#define AR_SIZE_CHECK     1
#define AR_SIZE_MAX       10000

/*
cpp98: 199711  _MSC_VER always 199711
cpp11: 201103
cpp14: 201402
cpp17: 201703L
C++20: 202002L
*/
#define AR_CPP20       1

#if !defined(__cplusplus) 
#error "not used for c++"
#elif __cplusplus >=202002L 
#define AR_CPP_STD_VER AR_CPP20
#else 
#error "c++20 and newer should be support"
#endif

#define WSTR( str )   L##str

// OS
// AR_PLATFORM_WINDOW
// AR_PLATFORM_ANDROID
// AR_PLATFORM_LINUX
// AR_PLATFORM_IPHONE
// AR_PLATFORM_MAC
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(	_WIN64)
#	define AR_PLATFORM_WINDOW 1
#	define VK_USE_PLATFORM_WIN32_KHR 1
#	define AR_MAX_PATH        MAX_PATH
#	define AR_ANSI_LINE_TERMINATOR "\r\n"
#	define AR_WIDE_LINE_TERMINATOR L"\r\n"
#elif defined(__ANDROID__)
#	define AR_PLATFORM_ANDROID 1
#	define VK_USE_PLATFORM_ANDROID_KHR 1
#	define AR_MAX_PATH  PATH_MAX
#	define AR_ANSI_LINE_TERMINATOR "\n"
#	define AR_WIDE_LINE_TERMINATOR L"\n"
#elif defined(linux) || defined(__linux) || defined(__linux__)
#   define AR_PLATFORM_LINUX   1
#	define VK_USE_PLATFORM_WAYLAND_KHR 1
#	define AR_MAX_PATH    PATH_MAX
#	define AR_ANSI_LINE_TERMINATOR "\n"
#	define AR_WIDE_LINE_TERMINATOR L"\n"
#elif  defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
 	#include "TargetConditionals.h"
    #if TARGET_OS_IPHONE && TARGET_OS_SIMULATOR
		// iphone simulator
        #define AR_PLATFORM_IPHONE 1
    #elif TARGET_OS_IPHONE && TARGET_OS_MACCATALYST
		// catalyst
        #define AR_PLATFORM_IPHONE 1
    #elif TARGET_OS_IPHONE
        #define AR_PLATFORM_IPHONE 1
    #else
        #define AR_PLATFORM_MAC    1
    #endif

#if AR_PLATFORM_IPHONE
#define VK_USE_PLATFORM_IOS_MVK 1
#elif AR_PLATFORM_MAC
#define VK_USE_PLATFORM_MACOS_MVK
#endif 

#	define AR_MAX_PATH    1024
#	define AR_ANSI_LINE_TERMINATOR "\r"
#	define AR_WIDE_LINE_TERMINATOR L"\r"
#else
#error "OS not support"
#endif
#if !defined(AR_PLATFORM_WINDOW)
#define AR_PLATFORM_WINDOW 0
#endif
#if !defined(AR_PLATFORM_ANDROID)
#define AR_PLATFORM_ANDROID 0
#endif
#if !defined(AR_PLATFORM_LINUX)
#define AR_PLATFORM_LINUX  0
#endif
#if !defined(AR_PLATFORM_IPHONE)
#define AR_PLATFORM_IPHONE 0
#endif
#if !defined(AR_PLATFORM_MAC)
#define AR_PLATFORM_MAC    0
#endif

// compiler
// AR_COMPILER_MSVC 
// AR_COMPILER_CLANG
// AR_COMPILER_GCC
// AR_COMPILER_INTEL
#if defined(_MSC_VER) && _MSC_VER >=1800
#	if defined(__clang__)
#		define AR_COMPILER_CLANG  1 //clang with msvc
#	else
#		define AR_COMPILER_MSVC   1
#	endif
#elif defined(__clang__) 
#define AR_COMPILER_CLANG         1
#elif defined(__GNUC__) && !defined(__ibmxl__)
#define AR_COMPILER_GCC           1
#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
#define AR_COMPILER_INTEL         1
#else
#error "compiler not support"
#endif

#if !defined(AR_COMPILER_MSVC)
#define AR_COMPILER_MSVC 0
#endif
#if !defined(AR_COMPILER_CLANG)
#define AR_COMPILER_CLANG 0
#endif
#if !defined(AR_COMPILER_GCC)
#define AR_COMPILER_GCC  0
#endif
#if !defined(AR_COMPILER_INTEL)
#define AR_COMPILER_INTEL 0
#endif

#define AR_CACHE_OPT                                           1

//archecture 64
#if defined(__x86_64__) || defined(_M_X64) || defined(_WIN64) || defined(__64BIT__) || defined(_LP64) || defined(__LP64__) \
  || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) || defined(__amd64__) || defined(__arm64__) || defined(__aarch64__)
#  define AR_ARCH64                                            1             
#else
#  define AR_ARCH64                                            0
#endif

//vector math support
#	if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) //intel or amd
#		define AR_SIMD_SSE  1                                         	 
#   elif  defined(_M_ARM) || defined(_M_ARM64) //mali 
#		define AR_SIMD_NEON 1                          
#	else
#		define AR_SIMD_NONE 1
#	endif
#if !defined(AR_SIMD_SSE)
#define AR_SIMD_SSE 0
#endif
#if !defined(AR_SIMD_NEON)
#define AR_SIMD_NEON  0
#endif
#if !defined(AR_SIMD_NONE)
#define AR_SIMD_NONE 0
#endif

//compile mode. debug or release
#if  defined(DEBUG) || defined(_DEBUG)
#define AR_DEBUG											1
#else 
#define AR_DEBUG                                            0
#endif 

//function like macro
#ifndef ar_likely
//gcc  or clang
#if AR_COMPILER_GCC || AR_COMPILER_CLANG
#define ARLikely(x)    __builtin_expect(!!(x), 1)
#define ARUnlikely(x)  __builtin_expect(!!(x), 0)
#else
#define ARLikely(x)   (x)
#define ARUnlikely(x) (x)
#endif
#endif

#if AR_COMPILER_MSVC
#	define AR_MS_ALIGN(n)                __declspec(align(n))
#	define AR_GCC_ALIGN(n) 
#	define AR_DLLEXPORT                  __declspec(dllexport)            //building as a library
#	define AR_DLLIMPORT	                 __declspec(dllimport)            //building with this library
#	define AR_C_DLL_EXPORT               extern "C" __declspec(dllexport) // export dll/so for c
#	define AR_DEPRECATED(version, msg)   __declspec(deprecated(msg "please update your code"))
#	define AR_CDECL	                     __cdecl			
#   define AR_FASTCALL                   __fastcall
#   define AR_VECTORCALL                 __vectorcall
#	define AR_STDCALL	                 __stdcall										
#	define AR_FORCEINLINE                __forceinline
#	define AR_FORCENOINLINE              __declspec(noinline)
#elif AR_COMPILER_GCC || AR_COMPILER_CLANG //linux or clang
#	define AR_MS_ALIGN(n) 
#	define AR_GCC_ALIGN(n)	              __attribute__((aligned(n)))
#	define AR_DLLEXPORT	                  __attribute__((visibility("default")))
#	define AR_DLLIMPORT 	              __attribute__((visibility("default")))
#	define AR_C_DLL_EXPORT                extern "C"
#	define AR_DEPRECATED(version, msg)	  __attribute__((deprecated(msg "please update your code")))
#	define AR_CDECL	  		
#	define AR_VARARGS    
#	define AR_STDCALL	  
#	define AR_FORCEINLINE inline        __attribute__ ((always_inline))
#	define AR_FORCENOINLINE             __attribute__((noinline))
#   define AR_DISABLE_WARNING(Msg)
#else ///mac
#	define AR_MS_ALIGN(n) 
#	define AR_GCC_ALIGN(n)	             __attribute__((aligned(n)))
#	define AR_DLLEXPORT	
#	define AR_DLLIMPORT
#	define AR_C_DLL_EXPORT     
#	define AR_DEPRECATED(version, msg)   __attribute__((deprecated(MESSAGE "please update your code")))
#	define AR_CDECL	  		
#	define AR_VARARGS    
#	define AR_STDCALL	  
#	if  defined(AR_DEBUG)
#		define AR_FORCEINLINE inline
#	else
#		define AR_FORCEINLINE inline     __attribute__ ((always_inline))
#	endif
#	define AR_FORCENOINLINE              __attribute__((noinline))	
#endif

//lib or dll import export
#define AR_STATIC
#if defined(AR_STATIC)
#	define AR_API 
#else
#	if defined(AR_EXPORT_DLL)
#		define AR_API AR_DLLEXPORT
#   else 
#		define AR_API AR_DLLIMPORT
#	endif
#endif

// app or plugin
#if !defined(AR_PLUGIN)
#define AR_APP 1
#else 
#define AR_APP 0
#endif

#if AR_COMPILER_MSVC
//4514: un-used inline function had been removed
//4710: function had not been inlined
//4505: un-reference local function had been removed
//4996: _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4514 4710 4505 4996)
#endif

// enable memory leak check
#if AR_COMPILER_MSVC && AR_DEBUG
//#define new  new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define ar_open_leak_check()                           \
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); \
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;                  \
	_CrtSetDbgFlag(tmpFlag);                          
#define ar_dump_leak() _CrtDumpMemoryLeaks()
#else
#define ar_open_leak_check()
#define ar_dump_leak()
#endif

#if AR_DEBUG
	#if AR_COMPILER_MSVC
		#define ARDebugBreak()      __debugbreak()
	#elif defined(__aarch64__)
		#if defined(__APPLE__)
			#define ARDebugBreak() __builtin_debugtrap()
		#else 
			#define ARDebugBreak() __asm__ volatile(".inst 0xd4200000")
		#endif 
	#elif defined(__i386__) || defined(__x86_64__)
		#define ARDebugBreak()     __asm__ volatile("int $0x03")
	#elif defined(__arm__) && !defined(__thumb__)
		#define ARDebugBreak()     __asm__ volatile(".inst 0xe7f001f0")
	#else 
		#include <signal.h>
		#define ARDebugBreak()     raise(SIGTRAP)
	#endif
	#define ARAssert(expr) if(!(expr)){ ARDebugBreak(); }
#else 
	#define ARDebugBreak()	
	#define ARAssert(expr)
#endif // end AR_DEBUG

#define PP_CONNECT2(x,y) PP_CONNECT2_HELPER1(x, y)
#define PP_CONNECT2_HELPER1(x, y ) PP_CONNECT2_HELPER2(x##y)
#define PP_CONNECT2_HELPER2(result ) result

#define AR_STRINGIZE(...) AR_STRINGIZE_FAST(__VA_ARGS__)
#define AR_STRINGIZE_FAST(...) #__VA_ARGS__

#define _AR ar3d
#define PROJECT_NAMESPACE_BEGIN namespace _AR{
#define PROJECT_NAMESPACE_END }

#define AR_TFN(T)  template<class...> class T
#define AR_TFN1(T) template<class, class...> class T

// big/little endian
AR_FORCEINLINE bool IsLittleEndian()
{
	int t = 1;
	return *((char*)&t) == 1;
}

#define AR_THIS_CLASS(ClassName)  using this_type = ClassName;
#define	AR_ATTRIBUTE(Type, Member)  \
			Type _## Member;\
			AR_FORCEINLINE this_type& set_ ## Member(Type const& Other){ \
				_## Member = Other; \
				return *this; } \
			AR_FORCEINLINE Type const& Member() const{\
				return _## Member; } \
			AR_FORCEINLINE Type& get_ ## Member() { \
					return _## Member; }