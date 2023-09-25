#pragma once

#include<string>
#include"core/type.h"
#include"../app_config.h"
#include"vulkan/vulkan.h"

PROJECT_NAMESPACE_BEGIN

enum class EWindowMode
{
    FullScreen,
    WindowFullScreen,
    Windowed,
    Max
};

enum class EVSyncMode{
    ON,
    OFF,
    Default
};
 

struct WindowProperties
{
    AR_THIS_CLASS(WindowProperties);
    AR_ATTRIBUTE(I32Rect, rect);
    AR_ATTRIBUTE(EWindowMode, mode);
    AR_ATTRIBUTE(EVSyncMode, vsync);
    AR_ATTRIBUTE(bool, resizable);
    AR_ATTRIBUTE(std::string, title);
};

template<typename Derive>
class CommonTopWindow
{
public:
    WindowProperties properties{};

    CommonTopWindow()
    {
        i32 width = GAppConfigs.get<i32>(AppConfigs::WinWidth, 1);
        i32 height = GAppConfigs.get<i32>(AppConfigs::WinHeight, 1);
        static std::string defaultName{ "Aurora3d" };
        std::string const& name = GAppConfigs.get<std::string>(AppConfigs::AppName, defaultName);
        properties.set_rect(I32Rect{ 0,0,width, height })
            .set_title(name)
            .set_vsync(EVSyncMode::ON)
            .set_resizable(true)
            .set_mode(EWindowMode::Windowed);
    }

    virtual ~CommonTopWindow()=default;

    //render
    VkSurfaceKHR createSurface(VkInstance instance, VkPhysicalDevice device) const {
        return VK_NULL_HANDLE;
    }

    void* getWindowHandle() { return nullptr; }

    virtual void createSwapChain(){}

    //window
    virtual void close() {};
    virtual bool shouldClose() const { return false; };
    virtual float getDPIFactor() const { return 1.0f; };
    virtual float getContentScalarFactor() const{
        return 1.0f;
    }
    
    I32Rect const& resize(I32Rect const& rect){
        if (properties.resizable())
        {
            properties.set_rect(rect);
        }
        return properties.rect();
    }

};


PROJECT_NAMESPACE_END
