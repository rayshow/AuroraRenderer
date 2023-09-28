#pragma once
#if AR_PLATFORM_ANDROID

#include <dlfcn.h>
#include"platform/common/vulkan_context.h"
#include"singleton.h"


#define GET_VK_BASIC_ENTRYPOINTS(Func)  \
				GGlobalCommands()->Func = (PFN_##Func)dlsym(libvulkan, #Func ); \
				ARCheck(GGlobalCommands()->Func!=nullptr);                     \
                if(GGlobalCommands()->Func==nullptr){ return false; }

PROJECT_NAMESPACE_BEGIN

struct AndroidVulkanContext
            : public VulkanContext<AndroidVulkanContext>
            , Singleton<AndroidVulkanContext>
{
    bool initBasicEntryPoints(){
        AR_LOG("vulkan context: load basic funtion");
        void* libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (!libvulkan) return false;
        ENUM_VK_ENTRYPOINTS_BASE(GET_VK_BASIC_ENTRYPOINTS)
        AR_LOG("vulkan context: load basic funtion succ");
        return true;
    }
};

PROJECT_NAMESPACE_END

#endif