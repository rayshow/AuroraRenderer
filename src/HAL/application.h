#pragma once
#include"../compile.h"

#if RS_PLATFORM_ANDROID
#include"android/android_application.h"
#include"android/android_window.h"
#else 
#error "unkown platform application"
#endif 
