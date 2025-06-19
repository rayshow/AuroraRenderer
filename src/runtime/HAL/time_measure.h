#pragma once
#include"type.h"

#if AR_PLATFORM_DEFINE == AR_PLATFORM_ANDROID
#include"android/android_time.h"
#else 
#error "unkown platform logger"
#endif 
