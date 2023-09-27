#pragma once
#include"core/compile.h"

#if RS_PLATFORM_ANDROID
#include"android/android_logger.h"
#elif RS_PLATFORM_WINDOW
#include"window/win_logger.h"
#else 
#error "unkown platform logger"
#endif 

PROJECT_NAMESPACE_BEGIN

template<ELogLevel level, typename... Args>
inline void LogFileAndStdout( Args&&... args){
    Logger::log<level>(std::forward<Args>(args)...);
    Logger::logToFile<level>( std::forward<Args>(args)...);
}

template<ELogLevel level, bool bWithLoc, typename... Args>
inline void RelogFileAndStdout(Args&&... args){
	Logger::relog<level, bWithLoc>(std::forward<Args>(args)...);
	Logger::relogToFile<level, bWithLoc>(std::forward<Args>(args)...);
}

#define AR_PRFIX "AR3D:%-7s:"
#define AR_FILE_LINE " @file: %s:line:%d"
#define AR_LOG(Level, Fmt,  ...)      LogFileAndStdout<ELogLevel::Level>(AR_PRFIX Fmt, GetLogLevelString(ELogLevel::Level), __VA_ARGS__)
#define AR_LOG_LOC(Level, Fmt, ...)   LogFileAndStdout<ELogLevel::Level>(AR_PRFIX Fmt AR_FILE_LINE, GetLogLevelString(ELogLevel::Level), __VA_ARGS__, __FILE__, __LINE__)
#define AR_LOG_FLUSH()                Logger::flush()

#define AR_REPRINT "%s"
#define AR_RELOG(Level,Fmt, ...)        RelogFileAndStdout<ELogLevel::Level, false>(AR_PRFIX AR_REPRINT, GetLogLevelString(ELogLevel::Level), "", 0, Fmt, __VA_ARGS__)
#define AR_RELOG_LOC(Level, Fmt, ...)   RelogFileAndStdout<ELogLevel::Level, true>(AR_PRFIX AR_REPRINT AR_FILE_LINE, GetLogLevelString(ELogLevel::Level), __FILE__, __LINE__, Fmt,  __VA_ARGS__)

PROJECT_NAMESPACE_END