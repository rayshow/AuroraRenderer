#pragma once
#if AR_PLATFORM_ANDROID

#include"platform/common/window.h"
#include"platform/common/application.h"
#include"vulkan_context.h"

PROJECT_NAMESPACE_BEGIN

 
class AndroidWindow:public Window 
{
public:

    AndroidWindow(ANativeWindow* inHandle, Application* inApplication, WindowProperties const& inProperties)
        : Window{inProperties}
        , handle{inHandle}
        , application{inApplication}
        , closeCalled{false}
    {}

    virtual VkSurfaceKHR createSurface(VkInstance instance, VkPhysicalDevice device) const override {
        if(instance == nullptr){
            return VK_NULL_HANDLE;
        }

        SAndroidSurfaceCreateInfoKHR createInfo;
        createInfo.window = handle;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        GInstanceCommands()->vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);
        return surface;
    }

    virtual void close() override{
        application->requestExit();
        closeCalled = true;
    }

    virtual bool shouldClose() const override{
        return closeCalled ? true: handle == nullptr;
    }
    
    virtual float getDPIFactor() const override {
        return 0.0f;
    }


private:
    ANativeWindow* handle;
    Application* application;
    bool closeCalled;
};

PROJECT_NAMESPACE_END

#endif