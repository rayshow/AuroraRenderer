#pragma once

#define VULKAN_GENERATED_BEGIN namespace vulkan {
#define VULKAN_GENERATED_END   }

#include "core/type.h"
#include "HAL/logger.h"

using uint8 = _AR::u8;
using uint32 = _AR::u32;
using int32 = _AR::i32;

template<typename T>
void ZeroVulkanStruct(T& t, VkStructureType type)
{
	memset(&t, 0, sizeof(T));
	t.sType = type;
}

#include "vk_global.h"

