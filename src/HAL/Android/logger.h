#pragma once

#include <android/log.h>
#include <cstdio>
#include <thread>
#include"../platform_def.h"
#include"../common/logger.h"


class AndroidLogger: public CommonLogger
{
private:
    static void vprintf(ELogLevel loglevel, const char* fmt, ...){
        va_list vargs;
        va_start(vargs, fmt);
        __android_log_vprint((int)loglevel, kLogPrefix, fmt, vargs);
        va_end(vargs);
    }
public:

    RS_FORCEINLINE static AndroidLogger& getInstance(){
        static AndroidLogger GLogger;
        return GLogger;
    }

    static bool initialize(const char* filepath){
        if(!CommonLogger::initialize(filepath)){
            log(ELogLevel::Error,"open log file:%s failed", filepath);
            return false;
        }
        return true;
    }

    static void finalize(){ CommonLogger::finalize(); }


    template<typename... Args>
    static void log(ELogLevel level, const char* fmt, Args... args){
        vprintf(level, fmt,  std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void logTrace(ELogLevel level, const char* file, int line, const char* fmt,Args... args){
        if(file){
            vprintf(level, fmt, file, line, std::forward<Args>(args)...);
        }
        vprintf(level, fmt,  std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void reprintfWithLoc(ELogLevel level, const char* externMsg, const char* file, int line, const char* fmt, Args... args){
        char buffer[512]={0};
        snprintf(buffer, 512, fmt, std::forward<Args>(args)...);
        vprintf(level, "%s %s      file:%s, line: %d", externMsg, buffer, file, line);
    }

    template<typename... Args>
    static void reprintf(ELogLevel level,const char* externMsg, const char* fmt, Args&&... args){
        char buffer[512]={0};
        snprintf(buffer, 512, fmt, std::forward<Args>(args)...);
        vprintf(level, "%s %s", externMsg, buffer);
    }
    
    
};

using ELogLevel = ELogLevel;
using Logger = AndroidLogger;
