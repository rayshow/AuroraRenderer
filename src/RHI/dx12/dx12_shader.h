#pragma once

#include"dx12_header.h"
#include"dx12_context.h"
#include "RHI/shader.h"

PROJECT_NAMESPACE_BEGIN

class DX12Shader: public Shader
{
private:
    TRefCountPtr<ID3DBlob> _object;
public:

    bool loadFromFile(WString const& path);

    void initialize();
    
};

PROJECT_NAMESPACE_END