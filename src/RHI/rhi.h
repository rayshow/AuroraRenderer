#pragma once

#include"HAL/logger.h"
#include"dx12/dx12_context.h"

PROJECT_NAMESPACE_BEGIN

class RHI
{
public:
    RHI() {}

    static bool initialize() {
        AR_LOG(Info, "RHI::initialize");
        
        if (!DX12Context::getInstance().initialize()) {
            AR_LOG(Info, "RHI::initialize DX12 Context Initialize failed!");
            return false;
        }
        return true;
    }

    static bool finalize() {
        AR_LOG(Info, "RHI::finalize");
    }
};

PROJECT_NAMESPACE_END