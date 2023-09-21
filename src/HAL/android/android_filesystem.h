#pragma once

#if RS_PLATFORM_ANDROID
#include<string>
#include<type_traits>
#include<limits>

// file operate
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>  
#include <sys/stat.h>
#include <sys/types.h>

#include"../../type.h"
#include"../../util.h"
#include"../common/file_protocol.h"
#include"../logger.h"
#include"platform/common/raw_string.h"
#include"platform/common/to_string_protocol.h"


PROJECT_NAMESPACE_BEGIN

// only reqiure path directories permission
struct AndroidFileStat{
    struct stat _stat;
    bool _bInitialized;

    AndroidFileStat()
        :_bInitialized{false}
    {}

    AndroidFileStat(std::string const& path)
        :_bInitialized{false}
    {
        setup(path);
    }

    void setup(std::string const& path){
        _stat.st_size = -1;
        int ret = stat(path.c_str(), &_stat);
        if(ret == 0){
            _bInitialized=true;
        }
    }

    bool isValid() const{
        return _bInitialized;
    }

    ptr::size_t size(){
        return _stat.st_size;
    }

    bool isDir() const {
        if(isValid())
            return S_ISDIR(_stat.st_mode);
        return false;
    }

    bool isFile() const{
        if(isValid())
            return S_ISREG(_stat.st_mode);
        return false;
    }

    std::string getCreateTime() const{
        struct tm *tm;
        tm = localtime(&_stat.st_mtime);
        char buff[128]{0};
        strftime(buff, sizeof(buff), "%Y%m%d_%H_%M_%S", tm);
        return std::string{buff};
    }
};

// read optional / array stra
enum FileReadMemoryStrategy{
    Outside,
    Allocate
};

template<bool ReadWriteLength = true, bool ReadAllocate = true>
struct FileStrategy
{
    static constexpr bool bReadWriteLength = ReadWriteLength;
    static constexpr bool bReadAllocate = ReadAllocate;
};

template<bool ReadWriteLength = true, bool ReadAllocate = true >
constexpr FileStrategy<ReadWriteLength, ReadAllocate> kFileStrategry{};

constexpr FileStrategy<false, true> kNoReadWriteLength_Allocate{};
constexpr FileStrategy<false, false> kNoReadWriteLength_NoAllocate{};
constexpr FileStrategy<true, true> kReadWriteLength_Allocate{};
constexpr FileStrategy<true, false> kReadWriteLength_NoAllocate{};


struct ReadWriteOptional{};
constexpr ReadWriteOptional kReadWriteOptional{};

template<FileFixAccess Access = FileFixAccess::ReadWrite>
class AndroidFile{
private:
    int _handle;
    ptr::size_t _size;
    ptr::size_t _current;
    const char* _lastError;
    int _openFlag;

    using pchar = char const*;
    using FileType = AndroidFile;

    // error message
    enum{
        Success =0,
        NoNecessaryFlag=1,
    };
    static constexpr pchar kErrorMsg[2]={
        "is ok",
        "nececary flag not set, at least Read/Write/ReadWrite should be set"
    };

    static_assert(Access== FileFixAccess::Read || Access == FileFixAccess::Write || Access == FileFixAccess::ReadWrite,"unkown file acess");

    void _reset(){
        _handle = -1;
        _size=0;
        _current=0;
        _lastError=kErrorMsg[AndroidFile::Success];
    }

    void _setLastError(){
        _lastError = strerror(errno);
    }
public:
    AndroidFile()
        :_handle{-1}
        ,_current{0}
        ,_size{0}
        ,_lastError{kErrorMsg[AndroidFile::Success]}
        ,_openFlag{0}
    {}

    ~AndroidFile(){
        close();
    }

    AndroidFile(AndroidFile const&)=delete;
    void operator=(AndroidFile const&)=delete;

    AndroidFile(AndroidFile&& other)
        : _handle{other._handle}
        , _current{other._current}
        , _size{other._size}
        ,_lastError{other._lastError}
    {
        other._reset();
    }

    AndroidFile& operator=(AndroidFile&& other){
        _handle = other._handle;
        _current = other._current;
        _lastError = other._lastError;
        _size = other._size;
        other._reset();
        return *this;
    }

    bool isValid() const{
        return _handle!=-1;
    }

    ptr::size_t size() const {
        return _size;
    }

    void flush() {
        if(isValid()){
            fsync(_handle);
        }
    }

    void close(){
        flush();
        if(_handle!=-1){
            ::close(_handle);
        }
    }

    const char* getError() const{
        return _lastError?_lastError:"(None Error)";
    }

    bool create(std::string const& filepath){
        return open(filepath,EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists );
    }

    bool open(std::string const& filepath, EFileOption option){
        int flag=0;
        bool bHasRead = EnumHasAnyFlags(option, EFileOption::Read);
        bool bHasWrite = EnumHasAnyFlags(option, EFileOption::Write);

        if(bHasRead && bHasWrite){
            flag = O_RDWR;
        }else if(bHasRead){
            flag = O_RDONLY;
        }else if(bHasWrite){
            flag = O_WRONLY;
        }else{
            _lastError= kErrorMsg[AndroidFile::NoNecessaryFlag];
            return false;
        }
        bool bHasAppend = EnumHasAnyFlags(option, EFileOption::Append);
        if(bHasAppend && bHasWrite){
            flag |=O_APPEND;
        }

        bool bCreateIfNotExists = EnumHasAnyFlags(option, EFileOption::CreateIfNoExists);
        if(bCreateIfNotExists){
            flag |= O_CREAT;
        }

        bool bTrunc = EnumHasAnyFlags(option, EFileOption::Trunc);
        if(bTrunc){
            flag |= O_TRUNC;
        }

        _handle= ::open(filepath.c_str(), flag, 0777);
        if(_handle == -1)
        {
            _setLastError();
            return false;
        }
        _openFlag = flag;
        _size = lseek64(_handle, 0, SEEK_END);
		lseek64(_handle, 0, SEEK_SET);
        return true;
    }

    bool isReadWrite() const{
        return (_openFlag & O_RDWR)!=0;
    }

    bool isReadOnly() const{
        return (_openFlag & O_RDONLY)!=0;
    }

    bool isWriteOnly() const{
        return (_openFlag & O_WRONLY)!=0;
    }

    bool newWriteFile(std::string path){
        return open(path, EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists);
    }

    bool readFile(std::string path){
        return open(path, EFileOption::Read);
    }

    //On success, the number of bytes written is returned.  On error, -1 is returned, and errno is set to indicate the error
    uint64 rawWrite(const void* buf, uint64 count){
        int64 wroteCount=-1;
        if(isValid()){
            wroteCount = ::write(_handle, buf, count);
            if(wroteCount==-1){
                _setLastError();
            }
        }
        FILE_SYSTEM_DEBUG_LOG("==> rawWrite(buf:%p, count:%lld, wroteCount:%lld)", buf, count, wroteCount);
        return (uint64)wroteCount;
    }

    // only single object here, no array, no std::string
    template<
        typename T, 
        typename R= std::remove_reference_t<T>, 
        typename = std::enable_if_t< is_serializible_v<T,FileType> && !std::is_array_v<R> &&!is_std_string_v<T> >
    >
    FILE_SYSTEM_INLINE bool write(T&& t){
        static_assert(!has_polymorphism_serialize_v<T,FileType> 
            || std::is_pointer_v<std::remove_reference_t<T>>,"write object should he base type / implmenets deserialize/ polymorphismDeserialize ");

        if constexpr(has_to_string_v<R>){
            FILE_SYSTEM_DEBUG_LOG("==> write T:%s R:%s, value:%s", getTypeName(T), getTypeName(R), GET_OBJECT_RAW_STRING(t) );
        }
        else{
            FILE_SYSTEM_DEBUG_LOG("==> write T:%s R:%s", getTypeName(T), getTypeName(R) );
        }

        bool succ = true;
        // is pointer or ref, use polymorphism deserialize function
        if constexpr(has_polymorphism_serialize_v<T,FileType>){
            succ = DispatchPolymorphSerailizeFn< std::remove_pointer_t< R>, FileType>.serialize(std::forward<T>(t), this);
            FILE_SYSTEM_DEBUG_LOG("==> write polymorphism_serialize");
        }
        // no polymorphism function, directly call serialize
        else if constexpr(has_normal_serialize_v<R,FileType>){
            FILE_SYSTEM_DEBUG_LOG("==> write serialize");
            succ = t.serialize(this);
        // is base type?
        }else if constexpr(is_serializible_base_type_v<R>){
            FILE_SYSTEM_DEBUG_LOG("==> write pod");
            constexpr ptr::diff_t size = sizeof(T);
            succ = rawWrite(std::addressof(t), size) == size;
            
        }
        FILE_SYSTEM_DEBUG_LOG("==> write succ:%d", (int)succ );
        return succ;
    }

    // dynamic length T array  which T is basic type with real size
    template<
        typename T, 
        bool bWriteLength,
        bool bAllocate,
        typename = std::enable_if_t< is_serializible_v<T, FileType> || std::is_void_v<T> >
    >
    FILE_SYSTEM_INLINE bool write(T const* array, uint64 length, FileStrategy<bWriteLength, bAllocate> strategy) {
        FILE_SYSTEM_DEBUG_LOG("==> writeArray(T:%s array:%p, length:%lld, writeLength:%d)", getTypeName(T), array, length, (int)bWriteLength);
        length = array? length:0;
        if constexpr(bWriteLength){
            bool succ = write(length);
            if(!succ) return false;
        }
        if(length == 0) return true;

        if constexpr(has_normal_serialize_v<T,FileType> || has_polymorphism_serialize_v<T,FileType>){
            // struct object has serialize function array
            FILE_SYSTEM_DEBUG_LOG("==> writeArray<serialize | std::string>");
            // write each elements
            for(uint64 i = 0; i< length; ++i){
                if(!write(array[i])){
                    return false;
                }
            }
            return true;
        }
        else if constexpr(std::is_void_v<T>){
            // raw bytes
            return length == rawWrite(array, length);
        }
        else if constexpr(is_raw_string_v<T>){
            // raw-string array
            FILE_SYSTEM_DEBUG_LOG("==> writeArray<rawString>");
            for(uint64 i = 0; i< length; ++i){
                if(!write(array[i], RawCharString::length(array[i]), strategy)){
                    return false;
                }
            }
            return true;
        }
        // is base object array
        else if constexpr(is_serializible_base_type_v<T>){
            FILE_SYSTEM_DEBUG_LOG("==> writeArray<pod>");
            constexpr int32 kTypeSize = sizeof(T);
            uint64 byteSize = kTypeSize * length;
             return byteSize == rawWrite( reinterpret_cast<void const*>(array), byteSize );
        }        
        return false;
    }

    // optional object | raw-string
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    FILE_SYSTEM_INLINE bool write(T const* object, ReadWriteOptional) {
        FILE_SYSTEM_DEBUG_LOG("==> writeOptional(object:%p, exists:%d)", object, (int)(object!=nullptr));
        if constexpr(is_char_v<T>){
            return write(object, RawCharString::length(object), kReadWriteLength_Allocate);
        }
        else{
            bool exists = object!=nullptr;
            bool succ = write(exists);
            if(!succ) return false;
            if(!exists) return true;
            return write(*object);
        }
    }

    // T[N] which T is basic type with real size
    template<typename T, int32 N, typename = std::enable_if_t< is_serializible_v<T, FileType>> >
    FILE_SYSTEM_INLINE bool write(T const (& array)[N]){
        FILE_SYSTEM_DEBUG_LOG("==> writeStaticArray(T:%s N:%d)", getTypeName(T), N);
        return write(array, N, kReadWriteLength_NoAllocate);
    }

    // std::string
    FILE_SYSTEM_INLINE bool write(std::string const& str){
        FILE_SYSTEM_DEBUG_LOG("==> write std::string: %s %d", str.c_str(), str.length() );
        return write(str.c_str(), str.length(), kReadWriteLength_NoAllocate );
    }

    // std::vector
    template<typename T, typename Allocator, typename = std::enable_if_t< is_serializible_v<T,FileType> || is_std_string_v<T> >>
    FILE_SYSTEM_INLINE bool write(std::vector<T, Allocator> const& array){
        FILE_SYSTEM_DEBUG_LOG("==> write std::vector<%s>: %d", getTypeName(T), (int32)array.size());
        return write(array.data(), array.size(), kReadWriteLength_NoAllocate);
    }

    FILE_SYSTEM_INLINE int64 seek(EFileSeek point, int64 offset){
        int64 seekOffset = -1;
        if(isValid()){
            #define rs_seek(...) lseek64(_handle, __VA_ARGS__, (ptr::diff_t)offset)
            switch(point){
                case EFileSeek::Current:    seekOffset = rs_seek(SEEK_CUR); break;
                case EFileSeek::End:        seekOffset = rs_seek(SEEK_END); break;
                case EFileSeek::SetPosition:seekOffset = rs_seek(SEEK_SET); break;
                default: break;
            }
            if(seekOffset==-1){
                _setLastError();
            }
            #undef rs_seek
        }
        return seekOffset;
    }

    template<typename T, typename P = std::enable_if_t<  is_serializible_base_type_v<T> > >
    bool writeOptionalArray(T const* ppArr, uint64 length){
        FILE_SYSTEM_DEBUG_LOG("==> writeOptionalArray<%s>: %d", getTypeName(T), length);
        for(uint64 i=0; i< length; ++i){
            write(ppArr[i], kReadWriteOptional);
        }
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
    FILE_SYSTEM_INLINE uint64 rawRead(void* buf, uint64 bytesCount){
        int64 readCount=-1;
        if(isValid()){
            readCount = ::read(_handle, buf, bytesCount);
            if(readCount == -1){
                _setLastError();
            }
        }
        FILE_SYSTEM_DEBUG_LOG("==> rawRead:%p byteCount:%d, readCount:%d", buf, bytesCount, readCount);
        return (uint64)readCount;
    }

    template<
        typename T, 
        typename R = std::remove_reference_t<T>,
        typename = std::enable_if_t< is_deserializible_v<T,FileType> && !std::is_array_v<T> &&!is_std_string_v<T> >
    >
    FILE_SYSTEM_INLINE bool read(T&& t){
        // T 
        static_assert(!has_polymorphism_deserialize_v<T,FileType> 
            || std::is_pointer_v<R>,"read object should he base type / implmenets deserialize/ polymorphismDeserialize ");

        FILE_SYSTEM_DEBUG_LOG("==> read T:%s R:%s", getTypeName(T), getTypeName(R));

        bool succ=true;
        if constexpr(has_polymorphism_deserialize_v<T,FileType>){
            FILE_SYSTEM_DEBUG_LOG("==> read polymorphism deserialize");
            succ = DispatchPolymorphSerailizeFn< std::remove_pointer_t< R >,FileType>.deserialize(std::forward<T>(t), this);
        }
        else if constexpr(has_normal_deserialize_v<T,FileType>){
            FILE_SYSTEM_DEBUG_LOG("==> read deserialize");
            succ = t.deserialize(this);
        }
        else if constexpr(is_serializible_base_type_v<T>){
            FILE_SYSTEM_DEBUG_LOG("==> read pod");
            constexpr ptr::diff_t size = sizeof(T);
            succ = this->rawRead(const_cast< std::remove_const_t<R>*>(std::addressof(t ) ), size) == size;
        }
        if constexpr(has_to_string_v<R>){
            FILE_SYSTEM_DEBUG_LOG("==> read T:%d, value:%s ", (int)succ, GET_OBJECT_RAW_STRING(t));
        }else{
            FILE_SYSTEM_DEBUG_LOG("==> read T:%d ", (int)succ);
        }
        
        return succ;
    }

    // object array
    template<
        typename T,
        typename Int, 
        bool bReadLength, 
        bool bAllocate,
        typename R = std::remove_const_t< T >,
        typename = std::enable_if_t< (is_serializible_v<T,FileType> || std::is_void_v<T>) && std::is_integral_v<Int> > 
    >
    FILE_SYSTEM_INLINE bool read(T*& array, Int& length, FileStrategy<bReadLength, bAllocate> strategy, bool assignLength =true, uint64 maxLength = std::numeric_limits<Int>::max()) {
        FILE_SYSTEM_DEBUG_LOG("==> readArray(array:%p, length:%lld, bRead:%d, maxLength:%lld)", array, (uint64)length, (int)bReadLength, maxLength);

        uint64 readLength = length; 
        if constexpr(bReadLength){
            bool success = read(readLength);
            if(!success) return false;
            if(assignLength){ length = readLength;}
            // zero length array
            if(readLength == 0){
                array = nullptr;
                return true;
            } 
        }
        rs_check(readLength > 0);
        R* buffer=nullptr;
        if constexpr(bAllocate){
            // allocate array 
            if constexpr(is_char_v<T>){
                // 0-terminated
                buffer = ptr::safe_new_default_array<R>(readLength+1);
            }else{
                buffer = ptr::safe_new_default_array<R>(readLength);
            }
        }
        else{
            buffer = array;
            // outside should allocate the memory
            rs_check(buffer!=nullptr);
        }
        bool succ = true;

        // struct has serialize | std::string
        if constexpr(has_normal_deserialize_v<T,FileType> || has_polymorphism_deserialize_v<T,FileType> || is_std_string_v<T> ){
            // struct has serialize/deserialize or std::string
            FILE_SYSTEM_DEBUG_LOG("==> readArray<deserialize | string>");
            for(uint64 i=0; i< readLength; ++i){
                if(!read(buffer[i])){
                    return false;
                }
            }
        }
        else if constexpr(std::is_void_v<T>){
            //raw bytes
            FILE_SYSTEM_DEBUG_LOG("==> readArray<rawBytes>");
            succ = rawRead( buffer, readLength);
        }
        else if constexpr(is_char_v<T>){
            //raw-string
            FILE_SYSTEM_DEBUG_LOG("==> readArray<char>");
            if(!rawRead( buffer, readLength)){
                return false;
            }
            buffer[readLength]=0; 
        }
        else if constexpr(is_raw_string_v<T>){
            // raw-string array
            FILE_SYSTEM_DEBUG_LOG("==> readArray<rawString>");
            uint32 tempLength=0;
            for(uint64 i = 0; i< readLength; ++i){
                // recursive call this
                if(!read(buffer[i], tempLength, strategy)){
                    return false;
                }
            }
        }
        else if constexpr(is_serializible_base_type_v<T>){
            // pod not upper type
            FILE_SYSTEM_DEBUG_LOG("==> readArray<pod>");
            constexpr int32 kTypeSize = sizeof(T);
            uint64 byteSize = kTypeSize * readLength;
            succ = (byteSize == rawRead( buffer, byteSize ));
        }
        array = buffer;
        return succ;
    }

    // a optional object
    template<
        typename T,
        typename = std::enable_if_t< is_serializible_v<T,FileType>> 
    >
    FILE_SYSTEM_INLINE bool read(T*& object, ReadWriteOptional) {
        FILE_SYSTEM_DEBUG_LOG("==> readOptional");
        if constexpr(is_char_v<T>){
            // raw-string
            uint32 len = 0;
            return read(object, len, kReadWriteLength_Allocate);
        }else if constexpr(!is_char_v<T>){
            // object pointer
            bool succ = true;
            bool exists = true;
            succ = read(exists);
            if(!succ) return false;
            // object not exists
            if(!exists){ 
                FILE_SYSTEM_DEBUG_LOG("==> Not Exists");
                object = nullptr;
                return true; 
            } 
            // need memory
            object = ptr::safe_new_default_array<T>(); 
            rs_check(object!=nullptr); 
            return read(*object);
        }
    }


    template<typename T, int32 N, typename = std::enable_if_t< is_serializible_base_type_v<T>> >
    FILE_SYSTEM_INLINE bool read(T (& array)[N]){
        FILE_SYSTEM_DEBUG_LOG("==> readStaticArray(N:%d)", N);
        uint64 length{0};
        T* addr = &array[0];
        return read( addr, length, kReadWriteLength_NoAllocate);
    }

    FILE_SYSTEM_INLINE bool read(std::string& str){
        FILE_SYSTEM_DEBUG_LOG("==> read std::string:(%d %s)", str.length(), str.c_str());

        uint64 length{0};
        bool succ = read(length);
        if(!succ) return false;
        if(length == 0) return true;
        str.resize((ptr::size_t)length);
        rs_check(str.size() == (ptr::size_t)length);
        return  length == rawRead( reinterpret_cast<void*>( str.data() ), length);
    }

    template<typename T, typename Allocator, typename = std::enable_if_t< is_deserializible_v<T,FileType> || is_std_string_v<T> >>
    FILE_SYSTEM_INLINE bool read(std::vector<T, Allocator>& array){
        FILE_SYSTEM_DEBUG_LOG("==> read std::vector<%s>:(%d)", getTypeName(T), array.size() );

        constexpr int32 kTypeSize = sizeof(T);
        uint64 length{0};
        bool success = read(length);
        if(!success) return false;
        if(length == 0) return true;
        array.resize(length);
        rs_check(array.size() == (ptr::size_t)length);
        if constexpr(has_normal_deserialize_v<T,FileType> || has_polymorphism_deserialize_v<T,FileType> || is_std_string_v<T> ){
            FILE_SYSTEM_DEBUG_LOG("==> read std::vector<deserialize | string>");
            for(auto&& item : array){
                if(!read(item)){
                    return false;
                }
            }
            return true;
        }
        else if constexpr(is_serializible_base_type_v<T>){
            FILE_SYSTEM_DEBUG_LOG("==> read std::vector<pod>");
            uint64 byteSize = kTypeSize * length;
            return byteSize == rawRead( reinterpret_cast< void*>( array.data()), byteSize );
        }
        return false;
    }

    template<typename T, typename P = std::enable_if_t<  is_serializible_base_type_v<T> > >
    bool readOptionalArray(T*& ppArr, uint64 length){
        FILE_SYSTEM_DEBUG_LOG("==> readOptionalArray<%s>: %d", getTypeName(T), length);
        for(uint64 i=0; i< length; ++i){
            read(ppArr[i], kReadWriteOptional);
        }
    }

};

// read only, write is forbidden
struct ReadonlyAndroidFile: private AndroidFile<FileFixAccess::Read>
{
    using FileType = ReadonlyAndroidFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T,FileType> >>
    friend RS_FORCEINLINE bool operator<<(ReadonlyAndroidFile* file, T&& t){
        return file->read(std::forward<T>(t));
    }   

    template<typename T, typename = std::enable_if_t< is_serializible_v<T,FileType> >>
    friend RS_FORCEINLINE bool operator<<(ReadonlyAndroidFile& file, T&& t ){
        return operator<<(&file, std::forward<T>(t));
    }
};

struct WriteonlyAndroidFile: private AndroidFile<FileFixAccess::Write>
{
    using FileType = ReadonlyAndroidFile;
public:
    template<typename T, typename = std::enable_if_t< is_serializible_v<T,FileType> >>
    friend RS_FORCEINLINE bool operator<<(WriteonlyAndroidFile* file, T&& t){
        return file->write(std::forward<T>(t));
    }   

    template<typename T, typename = std::enable_if_t< is_serializible_v<T,FileType> >>
    friend RS_FORCEINLINE bool operator<<(WriteonlyAndroidFile& file, T&& t ){
        return operator<<(&file, std::forward<T>(t));
    }
};

using File = AndroidFile<>;
using ReadonlyFile = ReadonlyAndroidFile;
using WriteonlyFile = WriteonlyAndroidFile;

class AndroidFileSystem{
    public:
    static File* openFile(std::string const& path, EFileOption option){
        if(isValidPath(path)){
            File file{};
            if(file.open(path, option)){
                return new File(std::move(file));
            }
        }
        return nullptr;
    }

    static File* createFile(std::string path){
        return openFile(path, EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists);
    }

    static File* openReadonlyFile(std::string path){
        return openFile(path, EFileOption::Read);
    }

    static bool isValidPath(std::string const& path){
        return path.size()>0;
    }

    static bool isFileExists(std::string const& path) {
        AndroidFileStat stat(path);
        return stat.isValid() && stat.isFile();
    }

    static bool renameFile(std::string const& oldfile, std::string const& newfile){
        if(isFileExists(oldfile) && !isFileExists(newfile)){
            return 0 == ::rename(oldfile.c_str(), newfile.c_str());
        }
        return false;
    }

    static bool isDirExists(std::string const& path){
        AndroidFileStat stat(path);
        return stat.isValid() && stat.isDir();
    }

    static bool deleteDir(std::string const& path){
        if(isValidPath(path)){
            return ::rmdir(path.c_str());
        }
        return false;
    }

    static bool createDir(std::string const& path){
        if(isValidPath(path)){
            return ::mkdir(path.c_str(), 0777) == 0 || (errno == EEXIST);
        }
        return false;
    }
 

    static const char* getLastError(){
        return strerror(errno);
    }

    static bool deleteFile(std::string filepath){
       if(isValidPath(filepath)){
            return ::unlink(filepath.c_str()) == 0;
        }
        return false;
    }


    static std::string getFileCreateTime(std::string const& filepath){
        AndroidFileStat stat(filepath);
        if(stat.isValid() && stat.isFile()){
            return stat.getCreateTime();
        }
        return std::string{};
    }

    static bool renameExistsFile(std::string const& file)
    {
        ptr::size_t pos = file.find_last_of("/");
        std::string path = file.substr(0, pos+1);
        std::string filename = file.substr(pos+1, file.size());
 
        std::string fullFilename = path + filename;
        if(isFileExists(fullFilename)){
            std::string lastLogFileName= path+"2_"+filename;
            if(isFileExists(lastLogFileName)){
                std::string filetime = getFileCreateTime(lastLogFileName);
                rs_check(filetime.length()>0);
                if(!renameFile(lastLogFileName, path+filetime+filename)) 
                    return false;
            }
            if(!renameFile(fullFilename, lastLogFileName)) {
                return false;
            }
        }
        return true;
    }

};

using FileSystem = AndroidFileSystem;

PROJECT_NAMESPACE_END

#endif
