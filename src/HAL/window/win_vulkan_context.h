#pragma once

#include<libloaderapi.h>
#include"../common/common_vulkan_context.h"
#include"core/util/singleton.h"

#define GET_VK_BASIC_ENTRYPOINTS(Func)  \
				GGlobalCommands()->Func = (PFN_##Func)GetProcAddress(libvulkan, #Func ); \
				rs_check(GGlobalCommands()->Func!=nullptr);                     \
                if(GGlobalCommands()->Func==nullptr){ return false; }

PROJECT_NAMESPACE_BEGIN

struct WindowVulkanContext
            : public VulkanContext<WindowVulkanContext>
            , Singleton<WindowVulkanContext>
{
    bool initBasicEntryPoints(){
        AR_LOG("vulkan context: load basic funtion begin");
        HMODULE libvulkan = LoadLibraryA("vulkan-1.dll");
        if (!libvulkan) return false;
        ENUM_VK_ENTRYPOINTS_BASE(GET_VK_BASIC_ENTRYPOINTS)
        AR_LOG("vulkan context: load basic funtion end");
        return true;
    }
};

PROJECT_NAMESPACE_END
