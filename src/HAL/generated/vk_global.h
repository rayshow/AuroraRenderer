#pragma once

#include"vk_dispatch_defs.h"
#include"HAL/logger.h"

inline VkGlobalDispatchTable* GGlobalCommands(){
    static VkGlobalDispatchTable dispatchTable;
    return &dispatchTable;
}

inline VkInstDispatchTable*& GInstanceCommandsRef(){
    static VkInstDispatchTable* dispatchTable = nullptr;
    return dispatchTable;
}

// VkInstance handle and batch VkCommand associate with it
inline VkInstDispatchTable* GInstanceCommands(){
    rs_assert(GInstanceCommandsRef() !=nullptr);
    return GInstanceCommandsRef();
}

// VkDevice handle and batch Array with VkCommand associate with it
constexpr int kMaxPhysicalDevices=4;
inline VkDevDispatchTable*& GDeviceCommandsRef(int index=0){
    static VkDevDispatchTable* dispatchTables[kMaxPhysicalDevices]{nullptr};
    return dispatchTables[index];
}

inline VkDevDispatchTable* GDeviceCommands(int index=0){
    rs_assert(GDeviceCommandsRef(index)!=nullptr);
    return GDeviceCommandsRef(index);
}

inline VkDevDispatchTable* GetDeviceCommands(VkDevice handle){
    for(int i=0; i<kMaxPhysicalDevices; ++i){
        if(GDeviceCommands(i)!=nullptr && GDeviceCommands(i)->handle == handle){
            return GDeviceCommands(i);
        }
    }
    return nullptr;
}
