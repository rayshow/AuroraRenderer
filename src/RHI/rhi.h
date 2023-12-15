#pragma once

#include"HAL/logger.h"

class RHI
{
public:
    RHI();

    bool initialize() {
        AR_LOG(Info, "RHI::initialize");
    }

    bool finalize() {
        AR_LOG(Info, "RHI::finalize");
    }
};