#pragma once
#include"../compile.h"

#if AR_PLATFORM_DEFINE == AR_PLATFORM_ANDROID
#include"android/android_global_argments.h"
#else 
#error "unkown platform logger"
#endif 
