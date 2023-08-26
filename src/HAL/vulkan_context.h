#pragma once
#include"compile.h"

#if RS_PLATFORM_DEFINE == RS_PLATFORM_ANDROID
#include"android/vulkan_context.h"
#else 
#error "unkown platform logger"
#endif 
