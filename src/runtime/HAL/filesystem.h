#pragma once
#include"core/compile.h"

#if AR_PLATFORM_WINDOW
#include"window/win_filesystem.h"
#elif AR_PLATFORM_ANDROID
#include"android/filesystem.h"
#else 
#error "unkown platform logger"
#endif 

PROJECT_NAMESPACE_BEGIN

// read only, write is forbidden
struct ReadonlyFile : private WindowFile<FileFixAccess::Read>
{
    using FileType = ReadonlyFile;

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend AR_FORCEINLINE bool operator<<(FileType* file, T&& t) {
        return file->read(std::forward<T>(t));
    }

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend AR_FORCEINLINE bool operator<<(FileType& file, T&& t) {
        return operator<<(&file, std::forward<T>(t));
    }
};

struct WriteonlyFile : private WindowFile<FileFixAccess::Write>
{
    using FileType = WriteonlyFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend AR_FORCEINLINE bool operator<<(FileType* file, T&& t) {
        return file->write(std::forward<T>(t));
    }

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend AR_FORCEINLINE bool operator<<(FileType& file, T&& t) {
        return operator<<(&file, std::forward<T>(t));
    }
};


PROJECT_NAMESPACE_END