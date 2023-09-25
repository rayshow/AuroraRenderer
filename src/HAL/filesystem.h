#pragma once
#include"core/compile.h"

#if RS_PLATFORM_WINDOW
#include"window/win_filesystem.h"
#elif RS_PLATFORM_ANDROID
#include"android/filesystem.h"
#else 
#error "unkown platform logger"
#endif 

PROJECT_NAMESPACE_BEGIN

// read only, write is forbidden
struct ReadonlyFile : private File<FileFixAccess::Read>
{
    using FileType = ReadonlyFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(ReadonlyAndroidFile* file, T&& t) {
        return file->read(std::forward<T>(t));
    }

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(ReadonlyAndroidFile& file, T&& t) {
        return operator<<(&file, std::forward<T>(t));
    }
};

struct WriteonlyAndroidFile : private AndroidFile<FileFixAccess::Write>
{
    using FileType = ReadonlyAndroidFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(WriteonlyAndroidFile* file, T&& t) {
        return file->write(std::forward<T>(t));
    }

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(WriteonlyAndroidFile& file, T&& t) {
        return operator<<(&file, std::forward<T>(t));
    }
};


PROJECT_NAMESPACE_END