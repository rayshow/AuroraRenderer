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


template<typename Plugin>
struct AppPluginRegister
{
    Plugin* _plugin{ nullptr };
    AppPluginRegister()
    {
        _plugin = new Plugin{};
        Application::getInstance().addPlugin(_plugin);
    }

    ~AppPluginRegister()
    {
        if (_plugin) delete _plugin;
    }
};

#define AR_REGISTER_PLUGIN(Class) static AppPluginRegister<Class> PluginRegisters ## __LINE__;

PROJECT_NAMESPACE_END