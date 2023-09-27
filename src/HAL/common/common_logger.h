#pragma once

#include <cstdio>
#include <stdarg.h>
#include <utility>

#include"../platform_def.h"

enum class ELogLevel{
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

class CommonLogger
{
protected:
    inline static FILE* GLogFile = nullptr;
    constexpr static  int kReprintBufferSize = 512;
 
    static void vprintf_file(const char* fmt, ...){
        if(GLogFile){
            va_list vargs;
            va_start(vargs, fmt);
            std::vfprintf(GLogFile, fmt, vargs);
            va_end(vargs);
            // write end of line
            static constexpr char endline[2]{"\n"};
            fwrite(endline, 1, 1, GLogFile);
        }
    }
public:
 
    static bool initialize(const char* filepath){
        if (!GLogFile){
            GLogFile = fopen(filepath,"wt+");
        }
        return GLogFile!=nullptr;
    }

    static void finalize(){
        if(GLogFile){
            fflush(GLogFile);
            fclose(GLogFile);
            GLogFile=nullptr;
        }
    }

    template< ELogLevel level, typename... Args>
    static void logToFile(const char* fmt, Args... args){
        vprintf_file( fmt,  std::forward<Args>(args)...);
    }

    static void flush(){
        if(GLogFile) {
            fflush(GLogFile);
        } 
    }

    template<ELogLevel level, bool bWriteLoc, typename... Args>
    static void relogToFile(const char* fmt1, const char* prefix,  const char* file, int line, const char* fmt2,  Args... args){
        char buffer[kReprintBufferSize]={0};
        snprintf(buffer, kReprintBufferSize, fmt2, std::forward<Args>(args)...);
        if constexpr (bWriteLoc)
        {
            vprintf_file(fmt1, prefix, buffer, file, line);
            return;
        }
        vprintf_file(fmt1, prefix, buffer);
    }
};
