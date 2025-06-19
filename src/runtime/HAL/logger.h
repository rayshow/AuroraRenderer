#pragma once
#include"core/compile.h"
#include"core/type.h"

PROJECT_NAMESPACE_BEGIN

enum class ELogLevel {
    Verbose = 0,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Count,
};

inline constexpr const char* GetLogLevelString(ELogLevel level) {
    constexpr const char* names[(int)ELogLevel::Count] = {
           "Verbose","Debug","Info","Warning","Error","Fatal"
    };
    return names[(int)level];
}

template<typename Derive>
class CommonLogger
{
protected:
    inline static FILE*       GLogFile = nullptr;
    inline static ELogLevel   GLogLevel = ELogLevel::Info;
    constexpr static  int kReprintBufferSize = 512;

    template<typename Char>
    static void vprintf_file(const Char* fmt, ...) {
        va_list vargs;
        va_start(vargs, fmt);
        output(GLogFile, fmt, vargs);
        Derive::output(GLogFile, fmt, vargs);
        va_end(vargs);
        endline();
        Derive::endline(GLogFile);
    }

public:
    template<typename Char>
    static void output(FILE* const  file, const Char* fmt, va_list& vargs) {
        if (GLogFile) {
            RawStrOps<Char>::vfprintf(GLogFile, fmt, vargs);
            fflush(GLogFile);
        }
    }

    static void endline()
    {
        constexpr char endline[2]{ "\n" };
        if (GLogFile) {
            fwrite(endline, 1, 1, GLogFile);
        }
    }


    static bool initialize(const char* filepath) {
        if (!GLogFile) {
            GLogFile = fopen(filepath, "wt+");
        }
        return GLogFile != nullptr;
    }

    static bool initialize(wchar_t const* filepath)
    {
        if (!GLogFile)
        {
            GLogFile = _wfopen(filepath, L"wt+");
        }
        return GLogFile != nullptr;
    }

    static void finalize() {
        if (GLogFile) {
            fflush(GLogFile);
            fclose(GLogFile);
            GLogFile = nullptr;
        }
    }

    template< ELogLevel level, typename Char, typename... Args>
    static void logToFile(const Char* fmt, Args&&... args) {
        if (GLogFile && (u32)GLogLevel > (u32)level) {
            vprintf_file(fmt, std::forward<Args>(args)...);
        }
        
    }

    static void flush() {
        if (GLogFile) {
            fflush(GLogFile);
        }
    }

    template<ELogLevel level, bool bWriteLoc, typename Char, typename... Args>
    static void relogToFile(const Char* fmt1, const Char* prefix, const Char* suffix, const Char* file, int line, const Char* fmt2, Args... args) {
        Char buffer[kReprintBufferSize] = { 0 };
        RawStrOps<Char>::printf(buffer, kReprintBufferSize, fmt2, std::forward<Args>(args)...);
        if constexpr (bWriteLoc)
        {
            vprintf_file(fmt1, prefix, buffer, suffix, file, line);
        }
        else {
            vprintf_file(fmt1, prefix, buffer, suffix);
        }
        
    }
};

class StdoutLogger : public CommonLogger< StdoutLogger>
{
public:
    using Super = CommonLogger< StdoutLogger>;

    template<typename Char>
    static void output(FILE* const file, const Char* fmt, va_list& vargs)
    {
        RawStrOps<Char>::vfprintf(stdout, fmt, vargs);
    }

    static void endline(FILE* const  file)
    {
        std::fprintf(stdout, "\n");
    }
};

#if AR_PLATFORM_ANDROID
#include <android/log.h>
#include <cstdio>
#include <thread>
#include"../platform_def.h"
#include"../common/logger.h"

class AndroidLogger : public CommonLogger
{
private:
    static void vprintf(ELogLevel loglevel, const char* fmt, ...) {
        va_list vargs;
        va_start(vargs, fmt);
        __android_log_vprint((int)loglevel, kLogPrefix, fmt, vargs);
        va_end(vargs);
    }
public:

    AR_FORCEINLINE static AndroidLogger& getInstance() {
        static AndroidLogger GLogger;
        return GLogger;
    }

    static bool initialize(const char* filepath) {
        if (!CommonLogger::initialize(filepath)) {
            log(ELogLevel::Error, "open log file:%s failed", filepath);
            return false;
        }
        return true;
    }

    static void finalize() { CommonLogger::finalize(); }


    template<typename... Args>
    static void log(ELogLevel level, const char* fmt, Args... args) {
        vprintf(level, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void logTrace(ELogLevel level, const char* file, int line, const char* fmt, Args... args) {
        if (file) {
            vprintf(level, fmt, file, line, std::forward<Args>(args)...);
        }
        vprintf(level, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void reprintfWithLoc(ELogLevel level, const char* externMsg, const char* file, int line, const char* fmt, Args... args) {
        char buffer[512] = { 0 };
        snprintf(buffer, 512, fmt, std::forward<Args>(args)...);
        vprintf(level, "%s %s      file:%s, line: %d", externMsg, buffer, file, line);
    }

    template<typename... Args>
    static void reprintf(ELogLevel level, const char* externMsg, const char* fmt, Args&&... args) {
        char buffer[512] = { 0 };
        snprintf(buffer, 512, fmt, std::forward<Args>(args)...);
        vprintf(level, "%s %s", externMsg, buffer);
    }
};

using Logger = AndroidLogger;

#elif AR_PLATFORM_WINDOW

class WindowLogger : public StdoutLogger {};
using Logger = WindowLogger;

#else 
#error "unkown platform logger"
#endif 

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