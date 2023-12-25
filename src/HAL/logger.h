#pragma once
#include"core/compile.h"

#if AR_PLATFORM_ANDROID
#include"android/android_logger.h"
#elif AR_PLATFORM_WINDOW
#include"window/win_logger.h"
#else 
#error "unkown platform logger"
#endif 

PROJECT_NAMESPACE_BEGIN

#define AR_PRFIX "AR3D:%-7s:"
#define AR_FILE_LINE " @file: %s:line:%d"
#define AR_EXPR_FILE_LINE " @expr:%s @file: %s:line:%d"
#define AR_LOG(Level, Fmt,  ...)      Logger::logToFile<ELogLevel::Level>(AR_PRFIX Fmt, GetLogLevelString(ELogLevel::Level), __VA_ARGS__)
#define AR_LOG_LOC(Level, Fmt, ...)   Logger::logToFile<ELogLevel::Level>(AR_PRFIX Fmt AR_FILE_LINE, GetLogLevelString(ELogLevel::Level), __VA_ARGS__, __FILE__, __LINE__)
#define AR_LOG_FLUSH()                Logger::flush()

#define AR_REPRINT "%s%s"
#define AR_RELOG(Level,Fmt, ...)        Logger::relogToFile<ELogLevel::Level, false>(AR_PRFIX AR_REPRINT, GetLogLevelString(ELogLevel::Level), "", "", 0, Fmt, __VA_ARGS__)
#define AR_RELOG_LOC(Level, Fmt, ...)   Logger::relogToFile<ELogLevel::Level, true>(AR_PRFIX AR_REPRINT AR_FILE_LINE, GetLogLevelString(ELogLevel::Level), "", __FILE__, __LINE__, Fmt,  __VA_ARGS__)

PROJECT_NAMESPACE_END