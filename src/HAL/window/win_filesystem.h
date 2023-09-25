#pragma once

#if 1

#include<string>
#include<type_traits>
#include<limits>
#include<fileapi.h>
#include"../common/common_filesystem.h"

PROJECT_NAMESPACE_BEGIN

struct WindowFileStat {
    WIN32_FILE_ATTRIBUTE_DATA _attrib{};
    bool _bInitialized{ false };

    WindowFileStat() = default;
    WindowFileStat(String const& path)
    {
        setup(path);
    }

    bool isValid() const {
        return _bInitialized && attrib.dwFileAttributes != INVALID_FILE_ATTRIBUTES;
    }

    bool isDir() const {
        return attrib.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }

    bool isFile() const {
        return isValid() && !(attrib.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool setup(String const& path) {
        return _bInitialized = GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &_attrib)
    }

    bool isReadOnly() {
        return attrib.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
    }

    bool setup(HANDLE handle) {
        _bInitialized = false;
        LPBY_HANDLE_FILE_INFORMATION Info;
        if (GetFileInformationByHandle(handle, &Info))
        {
            _attrib.dwFileAttributes = Info.dwFileAttributes;
            _attrib.ftCreationTime = Info.ftCreationTime;
            _attrib.ftLastWriteTime = Info.ftLastWriteTime;
            _attrib.ftLastAccessTime = Info.ftLastAccessTime;
            _attrib.nFileSizeLow = Info.nFileSizeLow;
            _attrib.nFileSizeHigh = Info.nFileSizeHigh;
            _bInitialized = true;
        }
        return _bInitialized;
    }

    i64 size() const {
        return (i64)attrib.nFileSizeHigh << 32 | (i64)attrib.nFileSizeLow;
    }
};


// reference https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
template<FileFixAccess Access = FileFixAccess::ReadWrite>
class WindowFile : public CommonFile<Access, WindowFile>  {
public:
    using Super = CommonFile<Access, WindowFile>;
    using This = WindowFile;

private:
    HANDLE  _handle;
    WindowFileStat _fileStat;

    static constexpr HANDLE kInvalidHandle = INVALID_HANDLE_VALUE;
    static constexpr i32 kOnceReadWriteBytes = std::numeric_limits<i32>::max();

    bool _setWindowErrorCode()
    {
        DWORD errorCode = GetLastError();
        switch (errorCode) {
        case ERROR_NEGATIVE_SEEK:
            _setLastError(ErrorCode::NegtiveSeek);
        default:
            _setLastError(ErrorCode::Custom, errorCode);
        }
        return false;
    }

    bool _setLastError(ErrorCode code) {
        return Super::_setLastError(code);
    }

    DWORD _getWindowSeekOption(EFileSeek seekOption) {
        switch (seekOption) {
        case EFileSeek::Begin:
            return FILE_BEGIN;
        case EFileSeek::Current:
            return FILE_CURRENT;
        case EFileSeek::End:
            return FILE_END;
        default:break;
        }
        return FILE_BEGIN;
    }


    void _resetHandle() {
        _handle = INVALID_HANDLE_VALUE;
    }

    void _reset() {
        _resetHandle();
        Super::_reset();
    }

public:
    WindowFile()
        : _handle{ kInvalidHandle }
        , Super {}
    {}

    ~WindowFile() {
        close();
    }

    WindowFile(WindowFile&& other)
        : _handle{ other._handle }
        , Super{std::move(other)}
    {
        other._reset();
    }


    bool isValid() const {
        return _handle != INVALID_HANDLE_VALUE;
    }

    This& operator=(This&& other) {
        _handle = other._handle;
        _resetHandle();
        Super::operator=(std::move(other));
        return *this;
    }

    void flush() {
        if (isValid()) {
            FlushFileBuffers(_handle);
        }
    }

    void close() {
        flush();
        if (isValid()) {
            CloseHandle(_handle);
            _reset();
        }
    }


    bool open(String const& filepath, EFileOption option) {
        DWORD access = 0;
        bool hasRead = EnumHasAnyFlags(option, EFileOption::Read);
        bool hasWrite = EnumHasAnyFlags(option, EFileOption::Write);

        DWORD shareMode = 0;
        DWORD accessMode = 0;
        if (hasRead) {
            shareMode |= FILE_SHARE_READ;
            accessMode |= GENERIC_READ;
        }
        else if (hasWrite) {
            accessMode |= GENERIC_WRITE;
        }
        else {
            return _setLastError(Super::NoNecessaryFlag);
        }

        DWORD createMode = 0;
        
        // device open only
        if (EnumHasAnyFlags(option, EFileOption::Device)) {
            createMode = OPEN_EXISTING;
        }
        else{ 
            bool bCreateNotExists = EnumHasAnyFlags(option, EFileOption::CreateIfNoExists);
            bool bTrunc = EnumHasAnyFlags(option, EFileOption::Read);
            if (!bCreateNotExists) {
                if (bTrunc) {
                    createMode = TRUNCATE_EXISTING;
                }
                else {
                    createMode = OPEN_EXISTING;
                }
            }
            else {
                createMode = OPEN_ALWAYS;
            }
        }

        _handle = CreateFileA(filepath.c_str(), shareMode, nullptr, createMode, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (!isValid())
        {
            return _setWindowErrorCode();
        }
        if (!_fileStat.setup(_handle)) {
            return _setWindowErrorCode();
        }

        bool bAppend = EnumHasAnyFlags(option, EFileOption::Append);
        _size = attrib.size();
        if (bAppend) {
            i64 offset = seek(EFileSeek::End, 0);
            if (offset < 0) return false;
            _current = offset;
        }
        else {
            _current = 0;
        }
        
        return true;
    }
    
    i64 seek(EFileSeek point, i64 offset) {
        rs_assert(isValid());
        DWORD high = offset >> 32;
        DWORD low = offset & 0xffffffff;
        DWORD lowSeekOffset = SetFilePointer(_handle, low, &high, _getWindowSeekOption(point));
        i64 seekOffset = high << 32 | lowRealOffset;
        if (INVALID_SET_FILE_POINTER == lowSeekOffset || seekOffset <0) {
            _setLastError(ErrorCode::InvalidSeek);
            return -1;
        }
        return seekOffset;
    }


    //On success, the number of bytes written is returned.  On error, -1 is returned, and errno is set to indicate the error
    FileSize rawWrite(const void* buf, FileSize bytesCount) {
        FileSize remainBytes = bytesCount;
        FileSize wroteBytes = 0;
        for (; remainBytes > kOnceReadWriteBytes; remainBytes -= kOnceReadWriteBytes)
        {
            i32 onceWroteBytes = 0;
            if (!WriteFile(_handle, buf + wroteBytes, kOnceReadWriteBytes, &onceWroteBytes, nullptr)) {
                return -1;
            }
            rs_assert(onceWroteBytes == kOnceReadWriteBytes);
            wroteBytes += kOnceReadWriteBytes;
        }
        if (remainBytes) {
            i32 onceWroteBytes = 0;
            if (!WriteFile(_handle, buf + wroteBytes, remainBytes, &onceWroteBytes, nullptr)) {
                return -1;
            }
            rs_assert(remainBytes == onceWroteBytes);
            wroteBytes += remainBytes;
        }
        rs_assert(wroteBytes == bytesCount);
        return wroteBytes;
    }

    /**
     * @brief
     *
     * n success, the number of bytes read is returned (zero indicates
     * end of file), and the file position is advanced by this number.
     * It is not an error if this number is smaller than the number of
     * bytes requested; this may happen for example because fewer bytes
     * are actually available right now (maybe because we were close to
     * end-of-file,
     */
    FILE_SYSTEM_INLINE u64 rawRead(void* buf, FileSize bytesCount) {
        FileSize remainBytes = bytesCount;
        FileSize readBytes = 0;
        for (; remainBytes > kOnceReadWriteBytes; remainBytes -= kOnceReadWriteBytes)
        {
            i32 onceReadBytes = 0;
            if (!ReadFile(_handle, buf + readBytes, kOnceReadWriteBytes, &onceReadBytes, nullptr)) {
                return -1;
            }
            rs_assert(onceReadBytes == kOnceReadWriteBytes);
            readBytes += kOnceReadWriteBytes;
        }
        if (remainBytes) {
            i32 onceReadBytes = 0;
            if (!WriteFile(_handle, buf + readBytes, remainBytes, &onceReadBytes, nullptr)) {
                return -1;
            }
            rs_assert(remainBytes == onceReadBytes);
            readBytes += remainBytes;
        }
        rs_assert(readBytes == bytesCount);
        return readBytes;
    }

};

// read only, write is forbidden
struct ReadonlyFile : private WindowFile<FileFixAccess::Read>
{
    using FileType = ReadonlyFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(FileType* file, T&& t) {
        return file->read(std::forward<T>(t));
    }

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(FileType& file, T&& t) {
        return operator<<(&file, std::forward<T>(t));
    }
};

struct WriteonlyFile : private WindowFile<FileFixAccess::Write>
{
    using FileType = WriteonlyFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(FileType* file, T&& t) {
        return file->write(std::forward<T>(t));
    }

    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    friend RS_FORCEINLINE bool operator<<(FileType& file, T&& t) {
        return operator<<(&file, std::forward<T>(t));
    }
};
using File = WindowFile<>;


class WindowFileSystem {
public:
    static File* openFile(String const& path, EFileOption option) {
        if (isValidPath(path)) {
            File file{};
            if (file.open(path, option)) {
                return new File(std::move(file));
            }
        }
        return nullptr;
    }

    static File* createFile(String path) {
        return openFile(path, EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists);
    }

    static File* openReadonlyFile(String path) {
        return openFile(path, EFileOption::Read);
    }

    static bool isValidPath(String const& path) {
        return path.size() > 0;
    }

    static bool isFileExists(String const& path) {
        WindowFileStat stat(path);
        return stat.isFile();
    }

    static bool renameFile(String const& oldfile, String const& newfile) {
        if (isFileExists(oldfile) && !isFileExists(newfile)) {
            return MoveFile(oldfile.c_str(), newfile.c_str());
        }
        return false;
    }

    static bool isDirExists(String const& path) {
        WindowFileStat stat(path);
        return stat.isValid() && stat.isDir();
    }

    static bool deleteDir(String const& path) {
        if (isValidPath(path)) {
            return ::RemoveDirectoryA(path.c_str());
        }
        return false;
    }

    static bool createDir(String const& path) {
        if (isValidPath(path)) {
            return CreateDirectoryA(path.c_str(), nullptr);
        }
        return false;
    }

    static String getLastError() {
        DWORD errorCode = GetLastError();
        WCHAR   wszMsgBuff[512];  // Buffer for text.
        DWORD   dwChars;  // Number of chars returned.
        // Try to get the message from the system errors.
        dwChars = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errorCode,
            0,
            wszMsgBuff,
            512,
            NULL);

        if (dwChars == 0) {
            return kEmptyString;
        }
        return String{ wszMsgBuff };
    }

    static bool deleteFile(String filepath) {
        if (isValidPath(filepath)) {
            return ::DeleteFileA(filepath.c_str()) == 0;
        }
        return false;
    }


    static String getFileCreateTime(String const& filepath) {
        WindowFileStat stat(filepath);
        if (stat.isValid() && stat.isFile()) {
            return stat.getCreateTime();
        }
        return String{};
    }

    static bool renameExistsFile(String const& file)
    {
        usize pos = file.find_last_of("/");
        String path = file.substr(0, pos + 1);
        String filename = file.substr(pos + 1, file.size());

        String fullFilename = path + filename;
        if (isFileExists(fullFilename)) {
            String lastLogFileName = path + "2_" + filename;
            if (isFileExists(lastLogFileName)) {
                String filetime = getFileCreateTime(lastLogFileName);
                rs_check(filetime.length() > 0);
                if (!renameFile(lastLogFileName, path + filetime + filename))
                    return false;
            }
            if (!renameFile(fullFilename, lastLogFileName)) {
                return false;
            }
        }
        return true;
    }

    static TOptional<StringView> getTempPath() {
        static char lpTempPathBuffer[MAX_PATH];
        //  Gets the temp path env string (no guarantee it's a valid path).
        DWORD dwRetVal = GetTempPathA(MAX_PATH, lpTempPathBuffer)
        if (dwRetVal > MAX_PATH || (dwRetVal == 0))
        {
            return TOptional<StringView>{};
        }
        return TOptional<StringView>{lpTempPathBuffer};
    }

};

using FileSystem = WindowFileSystem;

PROJECT_NAMESPACE_END

#endif
