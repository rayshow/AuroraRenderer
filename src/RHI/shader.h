#pragma once

#include "core/digest/SHA_hash.h"

PROJECT_NAMESPACE_BEGIN

class Shader
{
public:
    SHAHash    _hash;
    TArray<u8> _bytecodes;

public:
    
};


PROJECT_NAMESPACE_END