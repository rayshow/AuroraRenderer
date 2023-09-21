#pragma once

#if RS_PLATFORM_ANDROID

#include<android_native_app_glue.h>
#include"platform/common/application.h"
#include"window.h"
#include"vulkan_context.h"

PROJECT_NAMESPACE_BEGIN


inline char const* GetAppCmdString(int32_t cmd){
	#define RET_CMD_STRING(Enum) case Enum: return #Enum;
	switch(cmd){
		RET_CMD_STRING(APP_CMD_INIT_WINDOW)
		RET_CMD_STRING(APP_CMD_TERM_WINDOW)
		RET_CMD_STRING(APP_CMD_WINDOW_RESIZED)
		RET_CMD_STRING(APP_CMD_WINDOW_REDRAW_NEEDED)
		RET_CMD_STRING(APP_CMD_CONTENT_RECT_CHANGED)
		RET_CMD_STRING(APP_CMD_GAINED_FOCUS)
		RET_CMD_STRING(APP_CMD_LOST_FOCUS)
		RET_CMD_STRING(APP_CMD_CONFIG_CHANGED)
		RET_CMD_STRING(APP_CMD_LOW_MEMORY)
		RET_CMD_STRING(APP_CMD_START)
		RET_CMD_STRING(APP_CMD_RESUME)
		RET_CMD_STRING(APP_CMD_SAVE_STATE)
		RET_CMD_STRING(APP_CMD_PAUSE)
		RET_CMD_STRING(APP_CMD_STOP)
		RET_CMD_STRING(APP_CMD_DESTROY)
	}
	#undef RET_CMD_STRING
	return "None";
}

inline void onAppCmd(android_app *app, int32_t cmd){
	auto application = reinterpret_cast<Application*>(app->userData);
    rs_check(application!=nullptr);
	switch (cmd){
		case APP_CMD_INIT_WINDOW: {
			application->resize(ANativeWindow_getWidth(app->window),
			                 ANativeWindow_getHeight(app->window));
			application->setSurfaceReady();
			RS_LOG("APP_CMD_INIT_WINDOW");
			break;
		}
		case APP_CMD_CONTENT_RECT_CHANGED: {
			// Get the new size
			auto width  = app->contentRect.right - app->contentRect.left;
			auto height = app->contentRect.bottom - app->contentRect.top;
			application->resize(width, height);
			RS_LOG("APP_CMD_CONTENT_RECT_CHANGED:%d %d", width, height);
			break;
		}
		case APP_CMD_GAINED_FOCUS: {
			application->setFocus(true);
			RS_LOG("APP_CMD_GAINED_FOCUS");
			break;
		}
		case APP_CMD_LOST_FOCUS: {
			application->setFocus(false);
			RS_LOG("APP_CMD_LOST_FOCUS");
			break;
		case APP_CMD_TERM_WINDOW:
		default:
			RS_LOG("event:%s",GetAppCmdString(cmd));
		}
	}
}

inline int32 onInputEvent(android_app *app, AInputEvent *input_event){
    return 1;
}

inline void onContentRectChanged(ANativeActivity *activity, const ARect *rect){
	RS_LOG("ContentRectChanged: %p\n", static_cast<void *>(activity));
	struct android_app *app = reinterpret_cast<struct android_app *>(activity->instance);
	auto                cmd = APP_CMD_CONTENT_RECT_CHANGED;
	app->contentRect = *rect;
	if (::write(app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd)){
		RS_LOG("Failure writing android_app cmd: {}\n", strerror(errno));
	}
}

inline bool processAndroidEvent(android_app *app){
	android_poll_source *source;
	int ident;
	int events;
	while ((ident = ALooper_pollAll(0, nullptr, &events, (void **) &source)) >= 0) {
		if (source) {
			source->process(app, source);
		}
		if (app->destroyRequested != 0) {
			return false;
		}
	}
	return true;
}

class AndroidApplication: public Application 
{
public:
	void setAndroidAPP(android_app* inApp){
		app = inAPP;
	}

    virtual ~AndroidApplication() =default;


    // application
    virtual EExitCode initialize() override{

		// setup callbacks
        app->onAppCmd = onAppCmd;
        app->onInputEvent = onInputEvent;
        app->activity->callbacks->onContentRectChanged = onContentRectChanged;
        app->userData  = this;

		// workdir
		auto&& activity = getActivity();
		internalDataDir = std::string{activity->internalDataPath ? activity->internalDataPath:""}+"/";
		externalStorageDir = std::string{activity->externalDataPath ? activity->externalDataPath:""}+"/";
		tempDir = std::string{"/data/local/tmp/"};
		RS_LOG("app internalWorkDir:%s externalDir:%s, tempDir:%s", internalDataDir.c_str(), externalStorageDir.c_str(), tempDir.c_str() );
		
		// setup logger system
		
		std::string logFile = externalStorageDir+"log.txt";
        std::string const& root = getExternalStorageDirectory();
        std::string loggerFileName = externalStorageDir +"log.txt";
		FileSystem::renameExistsFile(loggerFileName);
        Logger::initialize(loggerFileName.c_str());

		RS_LOG("hello,log:");
        RS_LOG_FLUSH();


        EExitCode code = Application::initialize();
        if(code!=EExitCode::Success){
            return code;
        }
        RS_LOG("wait on window surface to be ready");
        do
        {   
            if(!processAndroidEvent(app))
            {
                RS_LOG("app destroyed by os!");
                return EExitCode::Close;
            }

        }while(!isSurfaceReady() || !app->window);

		// create window
		WindowProperties properties;
		properties.title = "Android RS Debug Player";
		window = std::make_unique<AndroidWindow>(app->window, this,  properties );
		if(!window){
			RS_LOG("create window failed");
			return EExitCode::Fatal;
		}

		if(!AndroidVulkanContext::getInstance().initialize()){
			RS_LOG("create vulkan context failed!");
			return EExitCode::Fatal;
		}

		RS_LOG("Application initialize succ!");
		return EExitCode::Success;
    }

    virtual void finalize(EExitCode code) override {
		// destroy 
		AndroidVulkanContext::getInstance().finalize();

		// wait util event finish
		if(!exitRequested()) requestExit();
		while(processAndroidEvent(app));
		RS_LOG("Application finalize called!");
	}

	virtual void tick(double tickTime) override{
		processAndroidEvent(app);
	}

	virtual void processEvents() override{ }

	virtual void requestExit() override {
		ANativeActivity_finish(getActivity());
        Application::requestExit();
    }

    //render
    virtual std::string const& getSurfaceExtension() const override{
		static std::string SurfaceExtension{VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
		return SurfaceExtension;
	}

    virtual void resize(uint32 width, uint32 height) override {
		if(window){
			WindowSize size{width, height};
			window->resize(size);
		}
	}

	virtual void* getNativeWindow() override {return getAndroidApp()->window;}
    virtual void* getNativeApp() override {return getAndroidApp();}

    // native
    android_app* getAndroidApp(){
		return app;
	}
    ANativeActivity* getActivity(){
		return app ? app->activity : nullptr;
	}
	virtual std::string const& getExternalStorageDirectory() override{
		return externalStorageDir;
	}
    virtual std::string const& getTempDirectory() override{
		return tempDir;
	}
    virtual std::string const& getPlatformName() override {
        static std::string platform{"[Unkown Platform]"};
        return platform;
    }

private:
    android_app *app{nullptr};
	std::unique_ptr<AndroidWindow> window{nullptr};
	std::string externalStorageDir;
	std::string tempDir;
	std::string internalDataDir;
};
PROJECT_NAMESPACE_END

#endif 