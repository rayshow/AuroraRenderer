#pragma once
#include"type.h"

#if RS_PLATFORM_DEFINE == RS_PLATFORM_ANDROID
#include"android/android_time.h"
#else 
#error "unkown platform logger"
#endif 
