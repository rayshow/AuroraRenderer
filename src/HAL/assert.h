#pragma once

#inclde"logger.h"

PROJECT_NAMESPACE_BEGIN

#if RS_DEBUG

#include"stack_walker.h"
inline void dumpStackFrame()
{
	char callStack[4096];
	_AR::GetCallStack(callStack);
	AR_LOG(Info, "Stack(%d):%s", RawStrOps<char>::length(callStack), callStack);
}

#if RS_COMPILER_MSVC
#include"cassert"
#define rs_check(condi) assert(condi)
#define rs_ensure(condi) assert(condi)
#else 
template<bool bFatal>
inline bool checkConditionWithLoc(bool condi, char const* expr, char const* file, int line) {
	if (!condi) {
		AR_LOG("Assertion failed:%s file: %s line: %d", expr, file, line);
		if constexpr (bFatal) {
			char callStack[4096];
			rs::GetCallStack(callStack);
			AR_LOG("Stack(%d):%s", strlen(callStack), callStack);
			rs_debugbreak();
		}
		RS_LOG_FLUSH();
	}
	return condi;
}

#define rs_check2(expr)  if(!(expr)){ RS_LOG("Assertion failed:%s file: %s line: %d", #expr, __FILE__, __LINE__);  rs_debugbreak(); }

#define rs_check(expr)   checkConditionWithLoc<true>(expr, #expr, __FILE__, __LINE__ )
#define rs_ensure(expr)  checkConditionWithLoc<false>(expr, #expr, __FILE__, __LINE__ )
#define rs_checkf(expr,  ...)   if(!(expr)){  RS_LOG( __VA_ARGS__); checkConditionWithLoc<true>(expr, #expr, __FILE__, __LINE__ );}
#define rs_ensuref(expr,  ...)  if(!(expr)){  RS_LOG( __VA_ARGS__); checkConditionWithLoc<false>(expr, #expr, __FILE__, __LINE__ );}
#endif
#else 
#define rs_check(condi) 
#define rs_ensure(condi)
#endif


PROJECT_NAMESPACE_END