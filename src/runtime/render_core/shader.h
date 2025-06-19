#pragma once

#include "digest/SHA_hash.h"
#include "HAL/assert.h"

PROJECT_NAMESPACE_BEGIN

using ByteCodes = TArray<u32>;

class Shader
{
private:
    SHAHash    _hash;
    ByteCodes  _bytecodes;

public:
    Shader(SHAHash& hash, ByteCodes&& codes)
        :_hash{ hash }
        , _bytecodes{ std::move(codes) }
    {}

    Shader(Shader&& other)
        :_hash{other._hash}
        , _bytecodes{std::move(other._bytecodes)}
    {}

    u32 const* getCode() const {
        return _bytecodes.data();
    }

    u32 getSize() const {
        return _bytecodes.size();
    }
};


PROJECT_NAMESPACE_END