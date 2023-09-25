#pragma once

#pragma once

#if 1

#include<string>
#include<type_traits>
#include<limits>

#include"core/type.h"
#include"core/util/enum_as_flag.h"
#include"file_protocol.h"


PROJECT_NAMESPACE_BEGIN

// read optional / array stra
enum FileReadMemoryStrategy {
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


struct ReadWriteOptional {};
constexpr ReadWriteOptional kReadWriteOptional{};





template<FileFixAccess Access, typename Derive>
class CommonFile {
public:
    using This = CommonFile;
    using FileType = Derive;
    using FileSize = i64;
    using FileOffset = i64;

    // error message
    enum EError : u8 {
        Success = 0,
        NoNecessaryFlag,
        NotExists,
        InvalidSeek,
        NegtiveSeek,
        Unkown,
        Custom,
        Max
    };

    static constexpr crawstr kErrorMsg[EError::Max] = {
        "is ok",
        "nececary flag not set, at least Read/Write/ReadWrite should be set",
        "file not exists",
        "invalid seek",
        "negtive seek",
        "custom error code",
        "Unkown error",
    };

    static_assert(Access == FileFixAccess::Read || Access == FileFixAccess::Write
        || Access == FileFixAccess::ReadWrite, "unkown file acess");

protected:
    i64         _size;
    i64         _current;
    EFileOption _option;
    const char* _lastErrorMsg;
    EError      _lastErrorCode;
    u32         _platformErrorCode;

    void _reset() {
        _size = 0;
        _current = 0;
        _option = EFileOption::None;
        _setLastError(EError::Success);
    }

    bool _setLastError(EError error, u32 platformCode = 0 ) {
        i32 iCode = (i32)errorCode;
        if (error < EError::Max) {
            _lastErrorCode = error;
            _lastErrorMsg = kErrorMsg[error];
            _platformErrorCode = platformCode;
        }
        return false;
    }

public:

    CommonFile(CommonFile const&) = delete;
    void operator=(CommonFile const&) = delete;

    i64 size() const {
        return _size;
    }

    i64 current() const {
        return _current;
    }

    const char* getError() const {
        return _lastErrorMsg ? _lastErrorMsg : "(None Error)";
    }

    u32 getPlatformErrorCode() const {
        return _platformErrorCode;
    }

    bool isReadable() const {
        return EnumHasAnyFlags(_option, EFileOption::Read);
    }

    bool isWritable() const {
        return EnumHasAnyFlags(_option, EFileOption::Write);
    }

    bool isReadWrite() const {
        return isReadable() && isWritable();
    }

    bool isReadOnly() const {
        return isReadable() && !isWritable();
    }

    bool isWriteOnly() const {
        return !isReadable() && isWritable();
    }

    bool create(std::string const& filepath) {
        return Derive::open(filepath, EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists);
    }

    bool open(String const& filepath, EFileOption option) {
        return false;
    }

    // should overwrite
    void flush() {
        rs_assert(false);
    }

    // should overwrite
    void close() {
        rs_assert(false);
    }

    // should overwrite
    FileSize seek(EFileSeek point, FileSize offset) {
        rs_assert(false);
        return -1;
    }

    // should overwrite
    FileSize deriveRawWrite(const void* buf, FileSize count) {
        rs_assert(false);
        return 0;
    }

    // should overwrite
    FileSize deriveRawRead(void* buf, FileSize bytesCount) {
        rs_assert(false);
        return 0;
    }

    //On success, the number of bytes written is returned.  On error, -1 is returned, and errno is set to indicate the error
    FileSize deriveRawWrite(const void* buf, FileSize bytesCount) {
        return Derive::deriveRawWrite(buf, bytesCount);
    }

    //On success, the number of bytes read is returned.  On error, -1 is returned, and errno is set to indicate the error
    FILE_SYSTEM_INLINE FileSize deriveRawRead(void* buf, FileSize bytesCount) {
        return Derive::deriveRawRead(buf, count);
    }

    bool newWriteFile(std::string path) {
        return open(path, EFileOption::Write | EFileOption::Trunc | EFileOption::CreateIfNoExists);
    }

    bool readFile(std::string path) {
        return open(path, EFileOption::Read);
    }

    // only single object here, no array, no std::string
    template<
        typename T,
        typename R = std::remove_reference_t<T>,
        typename = std::enable_if_t< is_serializible_v<T, FileType> && !std::is_array_v<R> && !is_std_string_v<T> >
    >
    FILE_SYSTEM_INLINE bool write(T&& t) {
        static_assert(!has_polymorphism_serialize_v<T, FileType>
            || std::is_pointer_v<std::remove_reference_t<T>>, "write object should he base type / implmenets deserialize/ polymorphismDeserialize ");

        if constexpr (has_to_string_v<R>) {
            FILE_SYSTEM_DEBUG_LOG("==> write T:%s R:%s, value:%s", getTypeName(T), getTypeName(R), GET_OBJECT_RAW_STRING(t));
        }
        else {
            FILE_SYSTEM_DEBUG_LOG("==> write T:%s R:%s", getTypeName(T), getTypeName(R));
        }

        bool succ = true;
        // is pointer or ref, use polymorphism deserialize function
        if constexpr (has_polymorphism_serialize_v<T, FileType>) {
            succ = DispatchPolymorphSerailizeFn< std::remove_pointer_t< R>, FileType>.serialize(std::forward<T>(t), this);
            FILE_SYSTEM_DEBUG_LOG("==> write polymorphism_serialize");
        }
        // no polymorphism function, directly call serialize
        else if constexpr (has_normal_serialize_v<R, FileType>) {
            FILE_SYSTEM_DEBUG_LOG("==> write serialize");
            succ = t.serialize(this);
            // is base type?
        }
        else if constexpr (is_serializible_base_type_v<R>) {
            FILE_SYSTEM_DEBUG_LOG("==> write pod");
            FileSize size = sizeof(T);
            succ = deriveRawWrite(std::addressof(t), size) == size;

        }
        FILE_SYSTEM_DEBUG_LOG("==> write succ:%d", (int)succ);
        return succ;
    }

    // dynamic length T array  which T is basic type with real size
    template<
        typename T,
        bool bWriteLength,
        bool bAllocate,
        typename = std::enable_if_t< is_serializible_v<T, FileType> || std::is_void_v<T> >
    >
    FILE_SYSTEM_INLINE bool write(T const* array, FileSize length, FileStrategy<bWriteLength, bAllocate> strategy) {
        FILE_SYSTEM_DEBUG_LOG("==> writeArray(T:%s array:%p, length:%lld, writeLength:%d)", getTypeName(T), array, length, (int)bWriteLength);
        length = array ? length : 0;
        if constexpr (bWriteLength) {
            bool succ = write(length);
            if (!succ) return false;
        }
        if (length == 0) return true;

        if constexpr (has_normal_serialize_v<T, FileType> || has_polymorphism_serialize_v<T, FileType>) {
            // struct object has serialize function array
            FILE_SYSTEM_DEBUG_LOG("==> writeArray<serialize | std::string>");
            // write each elements
            for (FileSize i = 0; i < length; ++i) {
                if (!write(array[i])) {
                    return false;
                }
            }
            return true;
        }
        else if constexpr (std::is_void_v<T>) {
            // raw bytes
            return length == deriveRawWrite(array, length);
        }
        else if constexpr (is_raw_string_v<T>) {
            // raw-string array
            FILE_SYSTEM_DEBUG_LOG("==> writeArray<rawString>");
            for (FileSize i = 0; i < length; ++i) {
                if (!write(array[i], RawCharString::length(array[i]), strategy)) {
                    return false;
                }
            }
            return true;
        }
        // is base object array
        else if constexpr (is_serializible_base_type_v<T>) {
            FILE_SYSTEM_DEBUG_LOG("==> writeArray<pod>");
            constexpr int32 kTypeSize = sizeof(T);
            FileSize byteSize = kTypeSize * length;
            return byteSize == deriveRawWrite(reinterpret_cast<void const*>(array), byteSize);
        }
        return false;
    }

    // optional object | raw-string
    template<typename T, typename = std::enable_if_t< is_serializible_v<T, FileType> >>
    FILE_SYSTEM_INLINE bool write(T const* object, ReadWriteOptional) {
        FILE_SYSTEM_DEBUG_LOG("==> writeOptional(object:%p, exists:%d)", object, (int)(object != nullptr));
        if constexpr (is_char_v<T>) {
            return write(object, RawCharString::length(object), kReadWriteLength_Allocate);
        }
        else {
            bool exists = object != nullptr;
            bool succ = write(exists);
            if (!succ) return false;
            if (!exists) return true;
            return write(*object);
        }
    }

    // T[N] which T is basic type with real size
    template<typename T, int32 N, typename = std::enable_if_t< is_serializible_v<T, FileType>> >
    FILE_SYSTEM_INLINE bool write(T const (&array)[N]) {
        FILE_SYSTEM_DEBUG_LOG("==> writeStaticArray(T:%s N:%d)", getTypeName(T), N);
        return write(array, N, kReadWriteLength_NoAllocate);
    }

    // std::string
    FILE_SYSTEM_INLINE bool write(std::string const& str) {
        FILE_SYSTEM_DEBUG_LOG("==> write std::string: %s %d", str.c_str(), str.length());
        return write(str.c_str(), str.length(), kReadWriteLength_NoAllocate);
    }

    // std::vector
    template<typename T, typename Allocator, typename = std::enable_if_t< is_serializible_v<T, FileType> || is_std_string_v<T> >>
    FILE_SYSTEM_INLINE bool write(std::vector<T, Allocator> const& array) {
        FILE_SYSTEM_DEBUG_LOG("==> write std::vector<%s>: %d", getTypeName(T), (int32)array.size());
        return write(array.data(), array.size(), kReadWriteLength_NoAllocate);
    }

    template<typename T, typename P = std::enable_if_t<  is_serializible_base_type_v<T> > >
    bool writeOptionalArray(T const* ppArr, FileSize length) {
        FILE_SYSTEM_DEBUG_LOG("==> writeOptionalArray<%s>: %d", getTypeName(T), length);
        for (FileSize i = 0; i < length; ++i) {
            write(ppArr[i], kReadWriteOptional);
        }
    }


    template<
        typename T,
        typename R = std::remove_reference_t<T>,
        typename = std::enable_if_t< is_deserializible_v<T, FileType> && !std::is_array_v<T> && !is_std_string_v<T> >
    >
    FILE_SYSTEM_INLINE bool read(T&& t) {
        // T 
        static_assert(!has_polymorphism_deserialize_v<T, FileType>
            || std::is_pointer_v<R>, "read object should he base type / implmenets deserialize/ polymorphismDeserialize ");

        FILE_SYSTEM_DEBUG_LOG("==> read T:%s R:%s", getTypeName(T), getTypeName(R));

        bool succ = true;
        if constexpr (has_polymorphism_deserialize_v<T, FileType>) {
            FILE_SYSTEM_DEBUG_LOG("==> read polymorphism deserialize");
            succ = DispatchPolymorphSerailizeFn< std::remove_pointer_t< R >, FileType>.deserialize(std::forward<T>(t), this);
        }
        else if constexpr (has_normal_deserialize_v<T, FileType>) {
            FILE_SYSTEM_DEBUG_LOG("==> read deserialize");
            succ = t.deserialize(this);
        }
        else if constexpr (is_serializible_base_type_v<T>) {
            FILE_SYSTEM_DEBUG_LOG("==> read pod");
            constexpr ptr::diff_t size = sizeof(T);
            succ = this->deriveRawRead(const_cast<std::remove_const_t<R>*>(std::addressof(t)), size) == size;
        }
        if constexpr (has_to_string_v<R>) {
            FILE_SYSTEM_DEBUG_LOG("==> read T:%d, value:%s ", (int)succ, GET_OBJECT_RAW_STRING(t));
        }
        else {
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
        typename = std::enable_if_t< (is_serializible_v<T, FileType> || std::is_void_v<T>) && std::is_integral_v<Int> >
    >
    FILE_SYSTEM_INLINE bool read(T*& array, Int& length, FileStrategy<bReadLength, bAllocate> strategy, bool assignLength = true, FileSize maxLength = std::numeric_limits<Int>::max()) {
        FILE_SYSTEM_DEBUG_LOG("==> readArray(array:%p, length:%lld, bRead:%d, maxLength:%lld)", array, (FileSize)length, (int)bReadLength, maxLength);

        FileSize readLength = length;
        if constexpr (bReadLength) {
            bool success = read(readLength);
            if (!success) return false;
            if (assignLength) { length = readLength; }
            // zero length array
            if (readLength == 0) {
                array = nullptr;
                return true;
            }
        }
        rs_check(readLength > 0);
        R* buffer = nullptr;
        if constexpr (bAllocate) {
            // allocate array 
            if constexpr (is_char_v<T>) {
                // 0-terminated
                buffer = ptr::safe_new_default_array<R>(readLength + 1);
            }
            else {
                buffer = ptr::safe_new_default_array<R>(readLength);
            }
        }
        else {
            buffer = array;
            // outside should allocate the memory
            rs_check(buffer != nullptr);
        }
        bool succ = true;

        // struct has serialize | std::string
        if constexpr (has_normal_deserialize_v<T, FileType> || has_polymorphism_deserialize_v<T, FileType> || is_std_string_v<T>) {
            // struct has serialize/deserialize or std::string
            FILE_SYSTEM_DEBUG_LOG("==> readArray<deserialize | string>");
            for (FileSize i = 0; i < readLength; ++i) {
                if (!read(buffer[i])) {
                    return false;
                }
            }
        }
        else if constexpr (std::is_void_v<T>) {
            //raw bytes
            FILE_SYSTEM_DEBUG_LOG("==> readArray<rawBytes>");
            succ = deriveRawRead(buffer, readLength);
        }
        else if constexpr (is_char_v<T>) {
            //raw-string
            FILE_SYSTEM_DEBUG_LOG("==> readArray<char>");
            if (!deriveRawRead(buffer, readLength)) {
                return false;
            }
            buffer[readLength] = 0;
        }
        else if constexpr (is_raw_string_v<T>) {
            // raw-string array
            FILE_SYSTEM_DEBUG_LOG("==> readArray<rawString>");
            uint32 tempLength = 0;
            for (FileSize i = 0; i < readLength; ++i) {
                // recursive call this
                if (!read(buffer[i], tempLength, strategy)) {
                    return false;
                }
            }
        }
        else if constexpr (is_serializible_base_type_v<T>) {
            // pod not upper type
            FILE_SYSTEM_DEBUG_LOG("==> readArray<pod>");
            constexpr int32 kTypeSize = sizeof(T);
            FileSize byteSize = kTypeSize * readLength;
            succ = (byteSize == deriveRawRead(buffer, byteSize));
        }
        array = buffer;
        return succ;
    }

    // a optional object
    template<
        typename T,
        typename = std::enable_if_t< is_serializible_v<T, FileType>>
    >
    FILE_SYSTEM_INLINE bool read(T*& object, ReadWriteOptional) {
        FILE_SYSTEM_DEBUG_LOG("==> readOptional");
        if constexpr (is_char_v<T>) {
            // raw-string
            uint32 len = 0;
            return read(object, len, kReadWriteLength_Allocate);
        }
        else if constexpr (!is_char_v<T>) {
            // object pointer
            bool succ = true;
            bool exists = true;
            succ = read(exists);
            if (!succ) return false;
            // object not exists
            if (!exists) {
                FILE_SYSTEM_DEBUG_LOG("==> Not Exists");
                object = nullptr;
                return true;
            }
            // need memory
            object = ptr::safe_new_default_array<T>();
            rs_check(object != nullptr);
            return read(*object);
        }
    }


    template<typename T, int32 N, typename = std::enable_if_t< is_serializible_base_type_v<T>> >
    FILE_SYSTEM_INLINE bool read(T(&array)[N]) {
        FILE_SYSTEM_DEBUG_LOG("==> readStaticArray(N:%d)", N);
        FileSize length{ 0 };
        T* addr = &array[0];
        return read(addr, length, kReadWriteLength_NoAllocate);
    }

    FILE_SYSTEM_INLINE bool read(std::string& str) {
        FILE_SYSTEM_DEBUG_LOG("==> read std::string:(%d %s)", str.length(), str.c_str());

        FileSize length{ 0 };
        bool succ = read(length);
        if (!succ) return false;
        if (length == 0) return true;
        str.resize((ptr::size_t)length);
        rs_check(str.size() == (ptr::size_t)length);
        return  length == deriveRawRead(reinterpret_cast<void*>(str.data()), length);
    }

    template<typename T, typename Allocator, typename = std::enable_if_t< is_deserializible_v<T, FileType> || is_std_string_v<T> >>
    FILE_SYSTEM_INLINE bool read(std::vector<T, Allocator>& array) {
        FILE_SYSTEM_DEBUG_LOG("==> read std::vector<%s>:(%d)", getTypeName(T), array.size());

        constexpr int32 kTypeSize = sizeof(T);
        FileSize length{ 0 };
        bool success = read(length);
        if (!success) return false;
        if (length == 0) return true;
        array.resize(length);
        rs_check(array.size() == (ptr::size_t)length);
        if constexpr (has_normal_deserialize_v<T, FileType> || has_polymorphism_deserialize_v<T, FileType> || is_std_string_v<T>) {
            FILE_SYSTEM_DEBUG_LOG("==> read std::vector<deserialize | string>");
            for (auto&& item : array) {
                if (!read(item)) {
                    return false;
                }
            }
            return true;
        }
        else if constexpr (is_serializible_base_type_v<T>) {
            FILE_SYSTEM_DEBUG_LOG("==> read std::vector<pod>");
            FileSize byteSize = kTypeSize * length;
            return byteSize == deriveRawRead(reinterpret_cast<void*>(array.data()), byteSize);
        }
        return false;
    }

    template<typename T, typename P = std::enable_if_t<  is_serializible_base_type_v<T> > >
    bool readOptionalArray(T*& ppArr, FileSize length) {
        FILE_SYSTEM_DEBUG_LOG("==> readOptionalArray<%s>: %d", getTypeName(T), length);
        for (FileSize i = 0; i < length; ++i) {
            read(ppArr[i], kReadWriteOptional);
        }
    }
};


PROJECT_NAMESPACE_END

#endif
