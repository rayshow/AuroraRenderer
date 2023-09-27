#pragma once

#include <cstdio>
#include <stdarg.h>
#include <utility>

#include"../platform_def.h"

enum class ELogLevel{
    Unkown =0,
    Default,
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Silent
};

class CommonLogger
{
protected:
    inline static FILE* GLogFile = nullptr;
 
    static void vprintf_file(ELogLevel level, const char* fmt, ...){
        if(GLogFile){
            va_list vargs;
            va_start(vargs, fmt);
            std::vfprintf(GLogFile, fmt, vargs);
            va_end(vargs);

            // write end of line
            char endline[2]{"\n"};
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

    template<typename... Args>
    static void logToFile(ELogLevel level, const char* fmt, Args... args){
        vprintf_file( level, fmt,  std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void logTraceToFile(ELogLevel level, const char* file, int line, const char* fmt,Args... args){
        if(file){
            vprintf_file( fmt, file, line, std::forward<Args>(args)...);
        }
        vprintf_file( level, fmt,  std::forward<Args>(args)...);
    }

    static void flush(){
        if(GLogFile) {
            fflush(GLogFile);
        } 
    }

    template<typename... Args>
    static void reprintfWithLocToFile(ELogLevel level, const char* externMsg, const char* file, int line, const char* fmt, Args... args){
        char buffer[512]={0};
        snprintf(buffer, 512, fmt, std::forward<Args>(args)...);
        vprintf_file(level, "%s %s      file:%s, line: %d", externMsg, buffer, file, line);
    }

      template<typename... Args>
    static void reprintfToFile(ELogLevel level,const char* externMsg, const char* fmt, Args&&... args){
        char buffer[512]={0};
        snprintf(buffer, 512, fmt, std::forward<Args>(args)...);
        vprintf_file(level, "%s %s", externMsg, buffer);
    }

};
