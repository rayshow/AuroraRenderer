#pragma once

#include<string>
#include<chrono>
#include<thread>
#include<vector>
#include<algorithm>
#include<variant>
#include<array>
#include<unordered_map>
#include<unordered_set>

#include"core/type.h"
#include"core/util/singleton.h"
#include"../logger.h"
#include"top_window.h"
#include"app_tickable.h"
#include"engine.h"

PROJECT_NAMESPACE_BEGIN

enum class EExitCode{
    Success,
    Help,
    Close,
    Fatal
};

enum class EFPS{
    FPS30,
    FPS60,
    FPS120,
    Unlimit,
    Count
};

struct Timer{
    using Clock = std::chrono::steady_clock;
    using Resolution = std::ratio<1,1000>;
    using TimerPoint = typename Clock::time_point;

    Timer()
        : started{false}
        , initializePoint{Clock::now()} 
    { }

    void start(){
        startPoint =  Clock::now();
        previousPoint = currentPoint = startPoint;
        started = true;
    }

    void stop(){
        started = false;
    }

    double tick(double tickTime = 0.0){
        if(!started) return 0.0;
        while(elapsed() < tickTime){
            std::this_thread::yield();
        }
        previousPoint = currentPoint;
        currentPoint = Clock::now();
        return std::chrono::duration<double, Resolution>(currentPoint - previousPoint).count();
    }

    double elapsed(){
        if(!started){
            return 0.0f;
        }
        return std::chrono::duration<double, Resolution>(Clock::now() - previousPoint).count();
    }
private:
    TimerPoint startPoint, currentPoint, previousPoint, initializePoint;
    bool started;
};

struct AppPlugin {
    virtual EExitCode initialize() = 0;
    virtual void tick(double) = 0;
    virtual void finalize() = 0;
};



class AppConfigs
{
    enum
    {
        WinWidth = 0,
        WinHeight,
        ProjectName,
        Max
    };
    using AppConfigVarient = std::variant<i64, f64, i32, f32, std::string>;

    std::unordered_set<std::string>                   switches;
    std::array< AppConfigVarient, AppConfigs::Max>    definedConfigs;
    std::unordered_map<std::string, AppConfigVarient> configs;
    
    template<typename T>
    T& get(i32 index) {
        return std::get<T>(definedConfigs[index]);
    }

    template<typename T>
    T* get(std::string const& name) {
        auto& found = configs.find(name);
        if (found == configs.end()) {
            return
        }
        return confi
    }

};
inline AppConfigs GAppConfigs;


class Application: public Singleton<Application>
{
public:
    static constexpr double kFPSTickTimes[(int)EFPS::Count]={1.0/30.0, 1.0 / 60.0, 1.0 / 120.0, 0.0};
  
    virtual ~Application() = default;

    // entrance
    static i32 GuardMain() {
        Application& app = Application::getInstance();
        auto code = app.initialize();
        if (code == EExitCode::Success) {
            code = app.mainLoop();
            app.finalize(code);
        }
        return (i32)code;
    }


    // app start
    virtual EExitCode initialize(){
        // timer
        timer.start();
        std::vector<AppPlugin*> initializedPlugins;
        auto finalizePlugins = [&initializedPlugins]() {
            for (auto&& plugin : initializedPlugins) {
                plugin->finalize();
            }
        };
        for (auto& plugin : _plugins) {
            EExitCode code = plugin->initialize();
            if (code != EExitCode::Success) {
                finalizePlugins();
                return code;
            }
            initializedPlugins.push_back(plugin);
        }
        return EExitCode::Success;
    }

    EExitCode mainLoop(){
        // main loop
        RS_LOG("main loop begin");
        setFPS(EFPS::FPS60);
        while(!bRequestExit && !GRequestExit){
            f64 realTickTime = timer.tick(tickTime);
            processEvents();
            tick(realTickTime);
            for(auto&& plugin: _plugins){
                plugin->tick(realTickTime);
            }
            if(bFatal){
                return EExitCode::Fatal;
            }
        }
        RS_LOG("main loop end");
        return EExitCode::Success;
    }

    virtual void tick(f64 tickTime) {};
    virtual void processEvents() {};
    virtual void finalize(EExitCode code) {};
    virtual void requestExit(){ bRequestExit = true; }
    bool exitRequested() const{ return bRequestExit;}

    void setFPS(EFPS inFPS){
        u32 ifps = (u32)fps;
        if( ifps< (u32)EFPS::Count){
            fps = inFPS;
            tickTime = kFPSTickTimes[ifps];
        }
    }

    //  render
    virtual void resize(u32 width, u32 height){}
    virtual std::string const& getSurfaceExtension() const{
        static std::string SurfaceExtension{"None"};
        return SurfaceExtension;
    }

    // platform
    virtual std::string const& getExternalStorageDirectory() { return None; }
    virtual std::string const& getTempDirectory() { return None; }
    virtual std::string const& getPlatformName(){ return None; }
    virtual void* getNativeWindow(){return nullptr;}
    virtual void* getNativeApp(){return nullptr;}

    void setSurfaceReady(){ bSurfaceReady = true; }
    bool isSurfaceReady() const { return bSurfaceReady;}
	void setFocus(bool bFocusState){ bHasFocus = bFocusState; }
private:
    bool bRequestExit{false};
    bool bFatal{false};
    bool bSurfaceReady{false};
    bool bHasFocus{false};
    EFPS fps{EFPS::Unlimit};
    double tickTime;
    Timer timer;
    std::vector<AppPlugin*> _plugins;

    inline static std::string None{"[None]"};
};

PROJECT_NAMESPACE_END
