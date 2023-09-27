#pragma once

#include"../common/common_logger.h"

class StdoutLogger: public CommonLogger
{
private:
    static void vprintf( FILE* output, const char* fmt, ...){
        va_list vargs;
        va_start(vargs, fmt);
        std::vfprintf(output, fmt, vargs);
        std::fprintf(output, "\n");
        va_end(vargs);
    }

    template<ELogLevel level>
    constexpr static FILE* getOutput() {
        if constexpr (level > ELogLevel::Debug) {
            return stderr;
        }
        return stdout;
    }

public:

    template<ELogLevel level, typename... Args>
    static void log(const char* fmt, Args... args){
        vprintf(getOutput<level>(), fmt,  std::forward<Args>(args)...);
    }

    template<ELogLevel level, bool bWriteLoc, typename... Args>
    static void relog(const char* fmt1, const char* prefix, const char* file, int line, const char* fmt2, Args... args) {
        char buffer[kReprintBufferSize] = { 0 };
        snprintf(buffer, kReprintBufferSize, fmt2, std::forward<Args>(args)...);
        if constexpr (bWriteLoc)
        {
            vprintf(getOutput<level>(), fmt1, prefix, buffer, file, line);
            return;
        }
        vprintf(getOutput<level>(), fmt1, prefix, buffer);
    }
};