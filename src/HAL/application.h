#pragma once
#include"../compile.h"

#if RS_PLATFORM_DEFINE == RS_PLATFORM_ANDROID
#include"android/application.h"
#include"android/window.h"
#else 
#error "unkown platform application"
#endif 
