
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

class WindowLogger
{
private:

    static void vprintf(int loglevel, const char* fmt, ...){
        va_list vargs;
        va_start(vargs, fmt);
        std::vfprintf(stderr, fmt, vargs);
        std::fprintf(stderr, "\n");
        va_end(vargs);
    }

public:

    template<typename... Args>
    static void log(ELogLevel level, const char* fmt, Args... args){
        vprintf((int)level, fmt,  std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void logTrace(ELogLevel level, const char* file, int line, const char* fmt,Args... args){
        if(file){
            vprintf((int)level, fmt, file, line, std::forward<Args>(args)...);
        }
        vprintf((int)level, fmt,  std::forward<Args>(args)...);
    }
};

using ELogLevel = ELogLevel;
using Logger = WindowLogger;