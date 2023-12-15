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
#include"top_window.h"
#include"engine.h"
#include"../filesystem.h"
#include"../logger.h"
#include"../vulkan_context.h"
#include"RHI/rhi.h"


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

template<typename Derive>
class CommonApplication: public Singleton<Derive>
{
public:
    using Super = CommonApplication<Derive>;
    static constexpr double kFPSTickTimes[(int)EFPS::Count]={1.0/30.0, 1.0 / 60.0, 1.0 / 120.0, 0.0};
    virtual ~CommonApplication() = default;

    // app start
    EExitCode initialize(){
        derive = static_cast<Derive*>(this);

        // dir and log system
        internalDataDir = GAppConfigs.get(AppConfigs::InnerDataDir, internalDataDir);
        externalStorageDir = GAppConfigs.get(AppConfigs::ExternalDataDir, externalStorageDir);
        tempDir = GAppConfigs.get(AppConfigs::TempDir, externalStorageDir);

        // setup logger system
        std::string loggerFileName = externalStorageDir + "log.txt";
        FileSystem::renameExistsFile(loggerFileName);
        Logger::initialize(loggerFileName.c_str());

        AR_LOG( Info, "app internalWorkDir:%s externalDir:%s, tempDir:%s", internalDataDir.c_str(), externalStorageDir.c_str(), tempDir.c_str());

        // setup vulkan system
        if (!VulkanContext::getInstance().initialize()) {
            AR_LOG(Info, "create vulkan context failed!");
            return EExitCode::Fatal;
        }



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
        AR_LOG(Info, "main loop begin");
        setFPS(EFPS::FPS60);
        while(!bRequestExit && !GRequestExit){
            f64 realTickTime = timer.tick(tickTime);
            processEvents();
            static_cast<Derive*>(this)->tick(realTickTime);
            for(auto&& plugin: _plugins){
                plugin->tick(realTickTime);
            }
            derive->tick(tickTime);
            if(bFatal){
                return EExitCode::Fatal;
            }
        }
        AR_LOG(Info, "main loop end");
        return EExitCode::Success;
    }

    void finalize(EExitCode code) {
        for (auto& plugin : _plugins) {
            plugin->finalize();
        }
        Logger::finalize();
    };

    void tick(f64 tickTime) {};
    void processEvents() {};
    
    void requestExit(){ bRequestExit = true; }
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
    virtual String const& getExternalStorageDirectory() { return externalStorageDir; }
    virtual String const& getTempDirectory() { return tempDir; }
    virtual String const& getPlatformName(){ return None; }
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
    double tickTime{};
    Timer timer{};
    TArray<AppPlugin*> _plugins{};
    String externalStorageDir{};
    String tempDir{};
    String internalDataDir{};
    //CRTP
    Derive* derive{};

    inline static String None{"[None]"};
};

PROJECT_NAMESPACE_END
