#pragma once

#include"logger.h"
#include"callstack.h"

PROJECT_NAMESPACE_BEGIN

#if AR_DEBUG

#if 0
#include"cassert"
#define ARCheck(condi) assert(condi)
#define rs_ensure(condi) assert(condi)
#else 

template<bool bFatal, bool bWithMsg, typename... Args>
inline bool CheckConditionWithLoc(char const* expr, char const* file, int line, const char* fmt, Args...args) {
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


#define ARCheck(expr)                   ARLikely(expr) || CheckConditionWithLoc<true,false>( #expr, __FILE__, __LINE__  , "")|| ARDebugBreakWithFalse() 
#define ARCheckFormat(expr, Fmt,  ...)  ARLikely(expr) || CheckConditionWithLoc<true,true>(#expr, __FILE__, __LINE__,Fmt,  __VA_ARGS__ )|| ARDebugBreakWithFalse() 

#define AREnsureNoBreak(expr)           (ARLikely(expr) || CheckConditionWithLoc<false,false>(#expr, __FILE__, __LINE__ , ""))
#define AREnsure(expr)                  (AREnsureNoBreak(expr) || ARDebugBreakWithFalse())
#define AREnsureFormat(expr, Fmt, ...)  (ARLikely(expr) || CheckConditionWithLoc<false,true>(#expr, __FILE__, __LINE__, Fmt, __VA_ARGS__ )  || ARDebugBreakWithFalse())



#endif
#else 
#define ARCheck(condi)  condi
#define AREnsure(condi) condi
#define ARCheckWithMsg(expr, Fmt,  ...)   expr
#define AREnsureWithMsg(expr, Fmt,  ...)  expr
#endif


PROJECT_NAMESPACE_END