#pragma once

#include "core/type.h"

PROJECT_NAMESPACE_BEGIN

struct DllHandle: public FRefCountedObject
{
    void* handle = nullptr;
    DllHandle(String const& filename);
    virtual ~DllHandle() override;
};

struct DXCWrapper
{
    DXCWrapper();
    ~DXCWrapper();
};

PROJECT_NAMESPACE_END