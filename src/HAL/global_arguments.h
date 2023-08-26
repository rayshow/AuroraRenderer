#pragma once
#include"../compile.h"

#if RS_PLATFORM_DEFINE == RS_PLATFORM_ANDROID
#include"android/global_argments.h"
#else 
#error "unkown platform logger"
#endif 
