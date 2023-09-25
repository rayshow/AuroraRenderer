#pragma once
#include"core/compile.h"

#if RS_PLATFORM_ANDROID
#include"android/android_logger.h"
#elif RS_PLATFORM_WINDOW
#include"window/win_logger.h"
#else 
#error "unkown platform logger"
#endif 

template<typename... Args>
inline void logFileAndStdout( Args&&... args){
        Logger::log(std::forward<Args>(args)...);
        Logger::logToFile( std::forward<Args>(args)...);
}

template<bool WithLoc, typename... Args>
inline void reprintfFileAndStdout(Args&&... args){
        if constexpr(WithLoc){
                Logger::reprintfWithLoc( std::forward<Args>(args)...);
                Logger::reprintfWithLocToFile( std::forward<Args>(args)...);
        }else{
                Logger::reprintf( std::forward<Args>(args)...);
                Logger::reprintfToFile( std::forward<Args>(args)...);
        }
}

template<typename... Args>
inline void reprintfFileAndStdout(Args&&... args){
        Logger::reprintfWithLoc( std::forward<Args>(args)...);
        Logger::reprintfWithLocToFile( std::forward<Args>(args)...);
}

#define RS_DEBUG_LOG(...)         logFileAndStdout(ELogLevel::Debug, __VA_ARGS__) //Logger::log(ELogLevel::Info,  __VA_ARGS__)

#define RS_LOG(...)               logFileAndStdout(ELogLevel::Info, __VA_ARGS__) //Logger::log(ELogLevel::Info,  __VA_ARGS__)
#define RS_LOG_FILE(...)          Logger::logToFile(ELogLevel::Info,__VA_ARGS__)

#define RS_LOG_FLUSH()            Logger::flush()

#define RS_RELOG(exMsg, ...)      reprintfFileAndStdout<false>(ELogLevel::Info, exMsg, __VA_ARGS__)
#define RS_RELOG_LOC(exMsg, ...)  reprintfFileAndStdout<true>(ELogLevel::Info, exMsg, __FILE__, __LINE__, __VA_ARGS__)


#if RS_DEBUG

#include"stack_walker.h"
inline void dumpStackFrame()
{
   char callStack[4096];
   _AR::GetCallStack(callStack);
   RS_LOG("Stack(%d):%s",strlen(callStack), callStack);
}

#if RS_COMPILER_MSVC
#include"cassert"
#define rs_check(condi) assert(condi)
#define rs_ensure(condi) assert(condi)
#else 
template<bool bFatal>
inline bool checkConditionWithLoc(bool condi, char const* expr, char const* file, int line) {
	if (!condi) {
		RS_LOG("Assertion failed:%s file: %s line: %d", expr, file, line);
		RS_LOG_FLUSH();
		if constexpr (bFatal) {
			char callStack[4096];
			rs::GetCallStack(callStack);
			RS_LOG("Stack(%d):%s", strlen(callStack), callStack);
			RS_LOG_FLUSH();
			rs_debugbreak();
		}
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

