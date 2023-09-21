#pragma once

#include<string>
#include"core/type.h"
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

struct I32Rect
{
    i32 x;
    i32 y;
    i32 width;
    i32 height;
};
 

struct WindowProperties
{
    I32Rect     _rect{ 0,0,1280,720 };
    EWindowMode _mode{ EWindowMode::Windowed };
    EVSyncMode  _vsync{ EVSyncMode::Default };
    bool        _resizable{ true };
    std::string _title{"None"};
};


class TopWindow
{
public:
    WindowProperties properties;

    TopWindow(WindowProperties const& inProperties)
        :properties{inProperties}
    {}

    virtual ~TopWindow()=default;

    //render
    virtual VkSurfaceKHR createSurface(VkInstance instance, VkPhysicalDevice device) const{
        return VK_NULL_HANDLE;
    }

    virtual void createSwapChain(){}

    //window
    virtual void close() =0;
    virtual bool shouldClose() const = 0;
    virtual float getDPIFactor() const = 0;
    virtual float getContentScalarFactor() const{
        return 1.0f;
    }
    
    WindowSize& resize(WindowSize const& size){
        if(properties.resizable)
            properties.size = size;
        return properties.size;
    }

};


PROJECT_NAMESPACE_END
