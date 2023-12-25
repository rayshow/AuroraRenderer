#pragma once
#include"core/type.h"

#if AR_PLATFORM_WINDOW
#include"window/win_application.h"
#elif AR_PLATFORM_ANDROID
#include"android/android_application.h"
#include"android/android_window.h"
#else 
#error "unkown platform application"
#endif 

PROJECT_NAMESPACE_BEGIN

// entrance
inline i32 GuardMain() {
    Application& app = Application::getInstance();
    auto code = app.initialize();
    if (code == EExitCode::Success) {
        code = app.mainLoop();
        app.finalize(code);
    }
    return (i32)code;
}

PROJECT_NAMESPACE_END