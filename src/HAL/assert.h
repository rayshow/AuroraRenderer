#pragma once

#include"logger.h"
#include"callstack.h"

PROJECT_NAMESPACE_BEGIN

#if AR_DEBUG

#if 0
#include"cassert"
#define rs_check(condi) assert(condi)
#define rs_ensure(condi) assert(condi)
#else 

template<bool bFatal, bool bWithMsg, typename... Args>
inline bool checkConditionWithLoc(char const* expr, char const* file, int line, const char* fmt, Args...args) {
	if constexpr (bFatal) {
		char callStack[4096] = {};
		GetCallStack(callStack);
		AR_LOG(Error, "Stack(%d):%s", RawStrOps<char>::length(callStack), callStack);
		Logger::relogToFile<ELogLevel::Fatal, true>(AR_PRFIX "Assertion failed:%s" AR_EXPR_FILE_LINE,
			GetLogLevelString(ELogLevel::Fatal), expr, file, line, fmt, std::forward<Args>(args)...);
	}
	else {
		Logger::relogToFile<ELogLevel::Error, true>(AR_PRFIX "Assertion failed:%s" AR_EXPR_FILE_LINE,
			GetLogLevelString(ELogLevel::Error), expr, file, line, fmt, std::forward<Args>(args)...);
	}
	AR_LOG_FLUSH();
	return false;
}

#define ARDebugBreakWithFalse() (ARDebugBreak(), false)


#define ARCheck(expr)                   ARLikely(expr) || checkConditionWithLoc<true,false>( #expr, __FILE__, __LINE__  , "")|| ARDebugBreakWithFalse() 
#define ARCheckFormat(expr, Fmt,  ...)  ARLikely(expr) || checkConditionWithLoc<true,true>(#expr, __FILE__, __LINE__,Fmt,  __VA_ARGS__ )|| ARDebugBreakWithFalse() 

#define AREnsure(expr)                  ARLikely(expr) || checkConditionWithLoc<false,false>(#expr, __FILE__, __LINE__ , "") || ARDebugBreakWithFalse()
#define AREnsureFormat(expr, Fmt, ...)  ARLikely(expr) || checkConditionWithLoc<false,true>(#expr, __FILE__, __LINE__, Fmt, __VA_ARGS__ )  || ARDebugBreakWithFalse()
#endif
#else 
#define ARCheck(condi) 
#define AREnsure(condi)
#define ARCheckWithMsg(expr, Fmt,  ...)
#define AREnsureWithMsg(expr, Fmt,  ...)
#endif


PROJECT_NAMESPACE_END