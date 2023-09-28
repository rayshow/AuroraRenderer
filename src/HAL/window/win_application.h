#pragma once


#include"../common/application.h"
#include"win_top_window.h"

PROJECT_NAMESPACE_BEGIN

// init / loop / exit
class WinApplication: public CommonApplication< WinApplication>
{
public:
    EExitCode initialize() {
        _window = std::make_shared<WinTopWindow>();
        _window->initialize();

        EExitCode code = Super::initialize();
        
        _window->show();
        return EExitCode::Success;
    }

    void tick(f64 time) {
        Super::tick(time);
        _window->tick(time);
    }

    void finalize(EExitCode exitCode) {
        Super::finalize(exitCode);
        _window->finalize();
    }
private:
    std::shared_ptr<WinTopWindow> _window{ nullptr };
};

using Application = WinApplication;

PROJECT_NAMESPACE_END