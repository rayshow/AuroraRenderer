#pragma once


#include"../common/application.h"
#include"win_top_window.h"

PROJECT_NAMESPACE_BEGIN

extern HINSTANCE GInstance;

// init / loop / exit
class WinApplication: public Application
{
public:
    WinApplication() {}

    EExitCode initialize() {
        _window = std::make_shared<WinTopWindow>("window", 800, 600);
        _window->initialize();
        _window->show();
        _engine.initialize();
        return EExitCode::Success;
    }


    virtual void tick(f64 time) {
        _window->tick();
    }
    void finalize(EExitCode exitCode) {
        _window->finalize();
    }
private:
    std::shared_ptr<WinWindow> _window{ nullptr };
};

PROJECT_NAMESPACE_END