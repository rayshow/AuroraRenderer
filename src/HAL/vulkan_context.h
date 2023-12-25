#pragma once
#include"core/compile.h"

#if AR_PLATFORM_WINDOW
#include"window/win_vulkan_context.h"
#elif AR_PLATFORM_ANDROID
#include"android/android_vulkan_context.h"
#else 
#error "unkown platform logger"
#endif 
