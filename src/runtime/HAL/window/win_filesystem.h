#pragma once
#include "HAL/assert.h"

#if 1

#include<string>
#include<type_traits>
#include<limits>
//#include"win_system_call.h"
#include"../common/common_filesystem.h"
#include"win_system_call.h"

PROJECT_NAMESPACE_BEGIN

struct WindowFileStat {
    WIN32_FILE_ATTRIBUTE_DATA _attrib{};
    bool _bInitialized{ false };

    WindowFileStat() = default;

    template<typename Str, ArCheckType(Str, is_string) >
    WindowFileStat(Str const& path)
    {
        setup(path);
    }

    bool isValid() const {
        return _bInitialized && _attrib.dwFileAttributes != INVALID_FILE_ATTRIBUTES;
    }

    bool isDir() const {
        return _attrib.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }

    bool isFile() const {
        return isValid() && !(_attrib.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    template<typename Str, ArCheckType(Str, is_string)>
    bool setup(Str const& filepath) {
        if constexpr(std::is_same_v<Str, String>)
        {
            return _bInitialized = GetFileAttributesExW(filepath.c_str(), GetFileExInfoStandard, &_attrib);
        }else
        {
            return _bInitialized = GetFileAttributesExA(filepath.c_str(), GetFileExInfoStandard, &_attrib);
        }
    }

    bool isReadOnly() {
        return _attrib.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
    }

    bool setup(HANDLE handle) {
        _bInitialized = false;
        BY_HANDLE_FILE_INFORMATION Info;
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
        return (i64)_attrib.nFileSizeHigh << 32 | (i64)_attrib.nFileSizeLow;
    }

    ReadableTime getTime(EGetFileTimeType type) const {
        FILETIME tm{};
        switch (type)
        {
            case EGetFileTimeType::Create: tm = _attrib.ftCreationTime;break;
            case EGetFileTimeType::LastAccess: tm = _attrib.ftLastAccessTime;break;
            case EGetFileTimeType::LastWrite: tm = _attrib.ftLastWriteTime;break;
            default: ARCheck(false); break;
        }
        
        SYSTEMTIME systemTime;
        FileTimeToSystemTime(&tm,&systemTime);
        ReadableTime readableTime{};
        readableTime.year = systemTime.wYear;
        readableTime.month = systemTime.wMonth;
        readableTime.day = systemTime.wDay;
        readableTime.hour = systemTime.wHour;
        readableTime.minute = systemTime.wMinute;
        readableTime.second = systemTime.wSecond;
        readableTime.millisecond = systemTime.wMilliseconds;
        return readableTime;
    }
};


// reference https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
template<FileFixAccess Access = FileFixAccess::ReadWrite>
class WindowFile : public CommonFile<Access, WindowFile<Access>>  {
public:
    using Super = CommonFile<Access, WindowFile>;
    using This = WindowFile;
    using EError = Super::EError;
    using FileSize = Super::FileSize;
private:
    HANDLE  _handle;
    WindowFileStat _fileStat;

    static constexpr HANDLE kInvalidHandle = INVALID_HANDLE_VALUE;
    static constexpr i32 kOnceReadWriteBytes = kIntMax<i32>;

    bool _setLastError(EError code) {
        return Super::_setLastError(code, code == EError::Platform ? GetLastError() : 0);
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

    template<typename Char>
    const Char* getPlatformError(Char* buf, u32 bufSize) const
    {
        u32 size = WinSystemCall::GetSystemMessage(GetLastError(), buf, bufSize);
        return size == 0 ? "System Message Not Found" : buf;
    }

    template<typename Str, ArCheckType(Str, is_string)>
    bool open(Str const& filepath, EFileOption option) {
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
            bool bTrunc = EnumHasAnyFlags(option, EFileOption::Trunc);
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
        
        if constexpr(is_wide_string_v<Str>) {
            _handle = CreateFileW(filepath.c_str(), accessMode, shareMode, nullptr, createMode, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        }else {
            _handle = CreateFileA(filepath.c_str(), accessMode, shareMode, nullptr, createMode, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        }

        if (!isValid())
        {
            _setLastError(EError::Platform);
            return false;
        }
        if (!_fileStat.setup(_handle)) {
            _setLastError(EError::Platform);
            return false;
        }

        bool bAppend = EnumHasAnyFlags(option, EFileOption::Append);
        this->_size = _fileStat.size();
        if (bAppend) {
            i64 offset = seek(EFileSeek::End, 0);
            if (offset < 0) return false;
            
            this->_current = offset;
        }
        else {
            this->_current = 0;
        }
        
        return true;
    }
    
    i64 seek(EFileSeek point, i64 offset) {
        ARAssert(isValid());
        LONG high = offset >> 32;
        LONG low = offset & 0xffffffff;
        DWORD lowSeekOffset = SetFilePointer(_handle, low, &high, _getWindowSeekOption(point));
        i64 seekOffset = (i64)high << 32 | (i64)lowSeekOffset;
        if (INVALID_SET_FILE_POINTER == lowSeekOffset || seekOffset <0) {
            _setLastError(EError::InvalidSeek);
            return -1;
        }
        return seekOffset;
    }


    //On success, the number of bytes written is returned.  On error, -1 is returned, and errno is set to indicate the error
    FileSize deriveRawWrite(const void* buf, FileSize bytesCount) {
        FileSize remainBytes = bytesCount;
        FileSize wroteBytes = 0;
        const u8* bytes = static_cast<const u8*>(buf);
        for (; remainBytes > kOnceReadWriteBytes; remainBytes -= kOnceReadWriteBytes)
        {
            DWORD onceWroteBytes = 0;
            if (!WriteFile(_handle, bytes + wroteBytes, kOnceReadWriteBytes, &onceWroteBytes, nullptr)) {
                return -1;
            }
            ARAssert(onceWroteBytes == kOnceReadWriteBytes);
            wroteBytes += kOnceReadWriteBytes;
        }
        if (remainBytes) {
            DWORD onceWroteBytes = 0;
            if (!WriteFile(_handle, bytes + wroteBytes, remainBytes, &onceWroteBytes, nullptr)) {
                return -1;
            }
            ARAssert(remainBytes == onceWroteBytes);
            wroteBytes += remainBytes;
        }
        ARAssert(wroteBytes == bytesCount);
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
    FILE_SYSTEM_INLINE i64 deriveRawRead(void* buf, FileSize bytesCount) {
        DWORD readBytes = 0;
        if (!ReadFile(_handle, buf, bytesCount, &readBytes, nullptr)) {
            _setLastError(EError::Platform);
            return -1;
        }
        ARAssert(readBytes == bytesCount);
        return readBytes;
    }
};

using File = WindowFile<>;

class WindowFileSystem {
public:

    template<typename Str, ArCheckType(Str, is_string)>
    static File* openFile(Str const& path, EFileOption option) {
        if (isValidPath(path)) {
            File file{};
            if (file.open(path, option)) {
                return new File(std::move(file));
            }
        }
        return nullptr;
    }

    template<typename Str, ArCheckType(Str, is_string) >
    static bool isValidPath(Str const& path) {
        return path.size() > 0;
    }

    static File* createFile(String path) {
        return openFile(path, EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists);
    }

    static File* openReadonlyFile(String path) {
        return openFile(path, EFileOption::Read);
    }

    template<typename Str, ArCheckType(Str, is_string) >
    static bool isFileExists(Str const& path) {
        WindowFileStat stat(path);
        return stat.isFile();
    }

    template<typename Str, ArCheckType(Str, is_string) >
    static bool renameFile(Str const& oldfile, Str const& newfile) {
        if (isFileExists(oldfile) && !isFileExists(newfile)) {
            if constexpr( std::is_same_v<Str, String> ) {
                return ::MoveFileW(oldfile.c_str(), newfile.c_str());    
            }else {
                return ::MoveFileA(oldfile.c_str(), newfile.c_str());
            }
        }
        return false;
    }
    
    template<typename Str, ArCheckType(Str, is_string) >
    static bool isDirExists(Str const& path) {
        WindowFileStat stat(path);
        return stat.isValid() && stat.isDir();
    }

    template<typename Str, ArCheckType(Str, is_string) >
    static bool deleteDir(Str const& path) {
        if (isValidPath(path)) {
            if constexpr( std::is_same_v<Str, String> ) {
                return ::RemoveDirectoryW(path.c_str());    
            }else{
                return ::RemoveDirectoryA(path.c_str());
            }
        }
        return false;
    }

    template<typename Str, ArCheckType(Str, is_string) >
    static bool createDir(Str const& path) {
        if (isValidPath(path)) {
            if constexpr( std::is_same_v<Str, String> ) {
                return ::CreateDirectoryW(path.c_str(), nullptr);    
            }else {
                return ::CreateDirectoryA(path.c_str(), nullptr);    
            }
        }
        return false;
    }

    template<typename Str, ArCheckType(Str, is_string) >
   static bool deleteFile(Str const& path) {
        if (isValidPath(path)) {
            if constexpr( std::is_same_v<Str, String> ) {
                return ::DeleteFileW(path.c_str(), nullptr);    
            }else {
                return ::DeleteFileA(path.c_str(), nullptr);    
            }
        }
        return false;
    }
    
    template<typename Str, ArCheckType(Str, is_string) >
    static Str getLastError() {
        return Str{};
        //return WinSystemCall::getLastErrorString<Str>();
    }

    template< typename FormatStr, typename FileStr, ArCheckType(FileStr, is_string), ArCheckType(FormatStr, is_string) >
    static FormatStr getFileCreateTime(FileStr const& filepath) {
        WindowFileStat stat(filepath);
        if (stat.isValid() && stat.isFile()) {
            ReadableTime time = stat.getTime(EGetFileTimeType::Create);
            if constexpr ( std::is_same_v<FormatStr, String> )
            {
                return String::format(128, L"%d%d%d-%d%d%d", time.year, time.month, time.day, time.hour, time.minute, time.second);
            }else
            {
                return String::format(128, "%d%d%d-%d%d%d", time.year, time.month, time.day, time.hour, time.minute, time.second);
            }
        }
        return FormatStr{};
    }

    template<typename Str, ArCheckType(Str, is_string) >
    static bool renameExistsFile(Str const& file)
    {
        usize pos = 0;
        if constexpr ( std::is_same_v<Str, String> ) {
            pos =  file.findLastOf(L'/');
        }else {
            pos = file.findLastOf('/');
        }
        Str path{ file.substr(0, pos + 1) };
        Str filename{ file.substr(pos + 1, file.size())};

        Str fullFilename{ path + filename};
        if (isFileExists(fullFilename)) {
            Str lastLogFileName = path;
            if constexpr ( std::is_same_v<Str, String> ) {
                lastLogFileName += AR_WIDE("_2")+ filename;
            }else {
                lastLogFileName += "_2"+ filename;
            }
            if (isFileExists(lastLogFileName)) {
                Str filetime = getFileCreateTime<Str>(lastLogFileName);
                ARAssert(filetime.length() > 0);
                Str fullpath{ path + filetime + filename};
                if (!renameFile(lastLogFileName, fullpath))
                    return false;
            }
            if (!renameFile(fullFilename, lastLogFileName)) {
                return false;
            }
        }
        return true;
    }

    static TOptional<AStringView> getTempPath() {
        static char lpTempPathBuffer[MAX_PATH];
        //  Gets the temp path env string (no guarantee it's a valid path).
        DWORD dwRetVal = GetTempPathA(MAX_PATH, lpTempPathBuffer);
        if (dwRetVal > MAX_PATH || (dwRetVal == 0))
        {
            return TOptional<AStringView>{};
        }
        return TOptional<AStringView>{lpTempPathBuffer};
    }

};

using FileSystem = WindowFileSystem;

PROJECT_NAMESPACE_END

#endif
