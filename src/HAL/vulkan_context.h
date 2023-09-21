#pragma once
#include"core/compile.h"

#if RS_PLATFORM_WINDOW
#include"window/window_vulkan_context.h"
#elif RS_PLATFORM_ANDROID
#include"android/android_vulkan_context.h"
#else 
#error "unkown platform logger"
#endif 
