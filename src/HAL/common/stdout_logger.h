#pragma once

#include"../common/common_logger.h"

class StdoutLogger: public CommonLogger< StdoutLogger>
{
public:
    using Super = CommonLogger< StdoutLogger>;

    static void output( FILE* const file, const char* fmt, va_list& vargs)
    {
        // to file
        Super::output(file, fmt, vargs);
        // to stdout
        std::vfprintf(stdout, fmt, vargs);
    }

    static void endline( FILE* const  file)
    {
        Super::endline(file);
        std::fprintf(stdout, "\n");
    }
};