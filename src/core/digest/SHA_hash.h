#pragma once

#include "core/type.h"

PROJECT_NAMESPACE_BEGIN

constexpr u32 kShaByteLength = 20;

class SHAHasher
{
protected:
    SHAHasher()=default;
public:
    virtual ~SHAHasher()=default;
    virtual void add(u8 const* buffer, usize size )=0;
    virtual void get(u8 (&bytes)[20])=0;
    static TUniquePtr<SHAHasher> create(); 
};

class SHAHash
{
public:
    
    static constexpr u32 kStringLength = kShaByteLength*2;
    alignas(u32) u8 hashBytes[kShaByteLength];

    SHAHash() { MemoryOps::zero(hashBytes);  }

    bool operator==(const SHAHash& other) const
    {
        return 0 == MemoryOps::compare( hashBytes, other.hashBytes);
    }

    bool operator<(const SHAHash& other) const
    {
        return MemoryOps::compare( hashBytes, other.hashBytes) < 0;
    }

    template<typename Str, ArCheckType(Str, is_string)>
    void toString(Str& str);

    template<typename Str, ArCheckType(Str, is_string)>
    void fromString(Str const& str);

    static SHAHash hash( u8 const* buffer, usize size );
};
PROJECT_NAMESPACE_END