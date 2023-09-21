#pragma once

#include<unordered_set>
#include<algorithm>
#include<deque>

#include"core/type.h"
#include"../logger.h"
#include"../common/to_string_protocol.h"
#include"../generated/vk_dispatch_defs.h"
#include"../generated/vk_api_enumerators.h"
#include"../generated/vk_command_defs.h"
#include"../generated/vk_enum_defs.h" 
#include"../generated/vk_wrap_defs.h"
#include"../generated/vk_extension_defs.h"
#include"../generated/vk_format_defs.h"

#undef max

using EmitFatalFn = void(*)(VkResult);
inline EmitFatalFn emitFatalCall = nullptr;
inline VkResult checkVulkanResultWithLoc(VkResult code, bool emitFatal, char const* expr, char const* file, int line) {
	if (code != VK_SUCCESS) {
		RS_LOG("invalid VkResult:%s at Expr:%s file:%s line:%d", GetVkResultString(code), expr, file, line);
		RS_LOG_FLUSH();
		if (emitFatal && (code == VK_ERROR_DEVICE_LOST || code == VK_ERROR_INITIALIZATION_FAILED)) {
			//emitFatalCall(code);
			if (emitFatalCall) emitFatalCall(code);
		}
	}
	return code;
}

// emit warning for all result not equal to VK_SUCCESS
#define EnsureVulkanResultLoc(Expr) checkVulkanResultWithLoc(Expr, false, #Expr,__FILE__,__LINE__)
#define EnsureVulkanResult(Expr) checkVulkanResultWithLoc(Expr, false, #Expr, "",__LINE__)
// fatal error for result is error
#define CheckVulkanResultLoc(Expr) checkVulkanResultWithLoc(Expr, true, #Expr,"",__LINE__)
#define CheckVulkanResult(Expr) checkVulkanResultWithLoc(Expr, true, #Expr,__FILE__,__LINE__)

#define GET_VK_DEVICE_ENTRYPOINTS(GetDeviceProcAddr, Device,  Name)   \
			DeviceCmds().Func = (PFN_vk ## Name) GetDeviceProcAddr(Device, vk ## Name) \
			GGlobalCommands()->Func = DeviceCmds().Func;

#define GET_VK_INSTANCE_ENTRYPOINTS(GetInstanceProcAddr, Instance,  Name) \
			GGlobalCommands()->Func = (PFN_vk ## Name) GetInstanceProcAddr(Instance, vk ## Name) \


PROJECT_NAMESPACE_BEGIN


template<typename T, typename Allocator, typename Fn, typename... Args>
inline void enumerateProperties(std::vector<T,Allocator>& array, Fn fn,  Args&&... args){
    VkResult ret;
    do{
        // get count first
        uint32 count = 0;
        CheckVulkanResult(ret = fn(std::forward<Args>(args)..., count, nullptr));
        if(count > 0){
            array.resize(count);
            CheckVulkanResult(ret = fn(std::forward<Args>(args)..., count, array.data()));
        }

    }while(ret == VK_INCOMPLETE);
}


inline uint32 GetMipSize(VkFormat format, VkExtent3D extent, uint32 arrayIndex, uint32 mipIndex)
{
    rs_check(mipIndex>=0 && mipIndex<=20);
    rs_check(arrayIndex>=0);
    VkFormatInfo& info = GetVkFormatInfo(format);
 
    VkExtent3D mipExtend{0,0,extent.depth};
	mipExtend.width  = std::max(extent.width >> mipIndex, (uint32)info.blockWidth);
	mipExtend.height = std::max(extent.height >> mipIndex, (uint32)info.blockHeight);

	uint32 numBlockX = (mipExtend.width + info.blockWidth - 1) / info.blockWidth;
	uint32 numBlockY = (mipExtend.height + info.blockHeight - 1) / info.blockHeight;
	// Size in bytes
	return numBlockX * numBlockY * info.blockSize;
}

inline uint32 GetMipSize(VkFormat format, VkExtent3D extent)
{
    VkFormatInfo& info = GetVkFormatInfo(format);
 
	uint32 numBlockX = (extent.width + info.blockWidth - 1) / info.blockWidth;
	uint32 numBlockY = (extent.height + info.blockHeight - 1) / info.blockHeight;
	// Size in bytes
	return numBlockX * numBlockY * info.blockSize;
}

inline uint32 GetImageSubresourceSize(VkFormat format, VkExtent3D extent, VkImageSubresourceRange& range)
{
    rs_check(range.layerCount>0 && range.levelCount>0);
    uint32 allMipSize = 0;
    for(uint32 i=0; i<range.levelCount; ++i){
        allMipSize += GetMipSize(format, extent, range.baseArrayLayer, range.baseMipLevel+i);
    }
    return allMipSize*range.layerCount;
}



inline VkBool32 VKAPI_PTR DebugReportFunction(
	VkDebugReportFlagsEXT			MsgFlags,
	VkDebugReportObjectTypeEXT		ObjType,
	uint64_t						SrcObject,
	size_t							Location,
	int32							MsgCode,
	const char*					LayerPrefix,
	const char*					Msg, void* UserData)
{
 
	const char* MsgPrefix = "UNKNOWN";
	if (MsgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		// Ignore some errors we might not fix...
		if (!strcmp(LayerPrefix, "Validation"))
		{
			if (MsgCode == 0x4c00264)
			{
				// Unable to allocate 1 descriptorSets from pool 0x8cb8. This pool only has N descriptorSets remaining. The spec valid usage text states
				// 'descriptorSetCount must not be greater than the number of sets that are currently available for allocation in descriptorPool'
				// (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkDescriptorSetAllocateInfo-descriptorSetCount-00306)
				return VK_FALSE;
			}
			else if (MsgCode == 0x4c00266)
			{
				// Unable to allocate 1 descriptors of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER from pool 0x89f4. This pool only has 0 descriptors of this type
				// remaining.The spec valid usage text states 'descriptorPool must have enough free descriptor capacity remaining to allocate the descriptor sets of
				// the specified layouts' (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkDescriptorSetAllocateInfo-descriptorPool-00307)
				return VK_FALSE;
			}
		}
		if (!strcmp(LayerPrefix, "SC"))
		{
			if (MsgCode == 3)
			{
				// Attachment N not written by fragment shader
				return VK_FALSE;
			}
			else if (MsgCode == 5)
			{
				// SPIR-V module not valid: MemoryBarrier: Vulkan specification requires Memory Semantics to have one of the following bits set: Acquire, Release, AcquireRelease or SequentiallyConsistent
				return VK_FALSE;
			}
		}
		if (!strcmp(LayerPrefix, "DS"))
		{
			if (MsgCode == 6)
			{
				auto* Found = strstr(Msg, " array layer ");
				if (Found && Found[13] >= '1' && Found[13] <= '9')
				{
					//#todo-rco: Remove me?
					// Potential bug in the validation layers for slice > 1 on 3d textures
					return VK_FALSE;
				}
			}
			else if (MsgCode == 15)
			{
				// Cannot get query results on queryPool 0x327 with index 193 as data has not been collected for this index.
				//return VK_FALSE;
			}
		}

		MsgPrefix = "ERROR";
	}
	else if (MsgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		MsgPrefix = "WARN";

		// Ignore some warnings we might not fix...
		// Ignore some errors we might not fix...
		if (!strcmp(LayerPrefix, "Validation"))
		{
			if (MsgCode == 2)
			{
				// fragment shader writes to output location 0 with no matching attachment
				return VK_FALSE;
			}
		}

		if (!strcmp(LayerPrefix, "SC"))
		{
			if (MsgCode == 2)
			{
				// fragment shader writes to output location 0 with no matching attachment
				return VK_FALSE;
			}
		}

	}
	else if (MsgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		MsgPrefix = "PERF";
		// Ignore some errors we might not fix...
		if (!strcmp(LayerPrefix, "SC"))
		{
			if (MsgCode == 2)
			{
				// vertex shader outputs unused interpolator
				return VK_FALSE;
			}
		}
		else if (!strcmp(LayerPrefix, "DS"))
		{
			if (MsgCode == 15)
			{
				// DescriptorSet previously bound is incompatible with set newly bound as set #0 so set #1 and any subsequent sets were disturbed by newly bound pipelineLayout
				return VK_FALSE;
			}
		}
	}
	else if (MsgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		MsgPrefix = "INFO";
	}
	else if (MsgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		MsgPrefix = "DEBUG";
	}
	else
	{
		rs_check(false);
	}


    RS_LOG_FILE("*** [%s:%s:%d] Obj 0x%p Loc %d %s\n", MsgPrefix, LayerPrefix, MsgCode, (void*)SrcObject, (uint32)Location, Msg);

    //static std::unordered_set<std::string> seenCodes;
	return VK_FALSE;
}

#define VkHandleProcess(oldHandle, newHandle, Type)  handleReplace((uint64_t)oldHandle, newHandle, Type, #oldHandle, __LINE__)

inline std::string GetDeviceHeapFlagString(VkMemoryPropertyFlags flag) {
	std::string ret;
	if ((flag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
		ret += " DEVICE_LOCAL ";
	}
	if ((flag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		ret += " HOST_VISIBLE ";
	}
	if ((flag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
		ret += " HOST_COHERENT ";
	}
	if ((flag & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
		ret += " HOST_CACHED ";
	}
	return ret;
}

inline void DumpPhysicalMemoryProp(VkPhysicalDeviceMemoryProperties& memoryProperties) {
	RS_LOG("\n** memory heap info **");
	RS_LOG("** type count:%d, heap cout:%d", memoryProperties.memoryTypeCount, memoryProperties.memoryHeapCount);

	for (uint32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
		VkMemoryType& type = memoryProperties.memoryTypes[i];
		RS_LOG("-- memory type[%d], heap index:%d, property:%s", i, type.heapIndex, GetDeviceHeapFlagString(type.propertyFlags).c_str());
	}
	for (uint32 i = 0; i < memoryProperties.memoryHeapCount; ++i) {
		VkMemoryHeap& heap = memoryProperties.memoryHeaps[i];
		RS_LOG("-- heap type[%d], heap size:%lld(%f MB), flagsd:%d", i, heap.size, (double)heap.size / (1024.0*1024.0), heap.flags);
	}
}



// one layer may has multi-extension
struct VulkanLayerExtensions
{
    VkLayerProperties layer;
    std::vector<VkExtensionProperties> extensions;
};

// single instance
struct VulkanInstance: public WInstance {
    std::vector<VulkanLayerExtensions> layerExtensions;
    std::vector<std::string> uniqueExtensions;

    bool GetCommandAddrs(){
        #define GET_INSTANCE_PROC_ADDR(Func)  \
                this->Func = (PFN_##Func)GGlobalCommands()->vkGetInstanceProcAddr(handle, #Func); \
				rs_ensure(this->Func!=nullptr);                     
        ENUMERATE_VK_INSTANCE_API(GET_INSTANCE_PROC_ADDR);
        GET_INSTANCE_PROC_ADDR(vkGetDeviceProcAddr);
        #undef GET_INSTANCE_PROC_ADDR
        return true;
    }
};

#define ENABLE_DEVICE_ID_PROPERTIES_KHR ENABLE_VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES || ENABLE_VK_KHR_EXTERNAL_FENCE_CAPABILITIES || ENABLE_VK_KHR_EXTERNAL_MEMORY_CAPABILITIES


enum class EVkQueueType:uint8{
    Graphics,
    Compute,
    Transfer,
    Sparse,
    Num
};

struct VulkanQueue{
    uint32 familyIndex;
    uint32 index;
    VkQueue handle;

    VulkanQueue()
        : familyIndex{kInvalidInteger<uint32>}
        , index{kInvalidInteger<uint32>}
        , handle{VK_NULL_HANDLE}
    {}

};

//multi-gpu
struct VulkanDevice: public WDevice{
    static constexpr int32 kQueueCount = (int32)EVkQueueType::Num;
    uint32 deviceIndex;
    std::vector<std::string> uniqueExtensions;
    std::vector<VulkanLayerExtensions> layerExtensions;
    WPhysicalDeviceProperties         deviceProperties;
    WPhysicalDeviceMemoryProperties  memoryProperties;

    // queue properties
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    VulkanQueue queues[kQueueCount];
    VulkanQueue& getQueue(EVkQueueType type){ return queues[(int)type]; }
    /*
    // physcal device properties
    WPhysicalDeviceProperties2 deviceProperties2;
    //WPhysicalDeviceIDPropertiesKHR deviceID;
    inline bool isDeviceIDPropertiesEnabled() const{
        return IsVkExtensionEnabled(VK_KHR_external_memory_capabilities) 
                || IsVkExtensionEnabled(VK_KHR_external_fence_capabilities) 
                || IsVkExtensionEnabled(VK_KHR_external_semaphore_capabilities);
    }
    */

    // shading rate
    bool GetCommandAddrs(){
        rs_check(GInstanceCommands()->vkGetDeviceProcAddr);
        #define GET_DEVICE_PROC_ADDR(Func)  \
                this->Func = (PFN_##Func)GInstanceCommands()->vkGetDeviceProcAddr(handle, #Func); \
				rs_ensure(this->Func!=nullptr);                     
        ENUMERATE_VK_DEVICE_API(GET_DEVICE_PROC_ADDR);
        #undef GET_DEVICE_PROC_ADDR
        return true;
    }

    void queryGPUProperties(uint32 index){
        rs_check( physicalHandle!=VK_NULL_HANDLE);
        // query physical device properties
        getPhysicalDeviceProperties(&deviceProperties);
        getPhysicalDeviceMemoryProperties(&memoryProperties);
        DumpPhysicalMemoryProp(memoryProperties);

        RS_LOG("** GPU info **");
        RS_LOG("* api version:%d", deviceProperties.apiVersion );
        RS_LOG("* driver version:%d", deviceProperties.driverVersion );
        RS_LOG("* vendorID:%d", deviceProperties.vendorID );
        RS_LOG("* deviceID:%d", deviceProperties.deviceID );
        RS_LOG("* deviceType:%d", deviceProperties.deviceType );
        RS_LOG("* deviceName:%s", deviceProperties.deviceName );
        RS_LOG("* pipelineCacheUUID:%s", deviceProperties.pipelineCacheUUID );
        RS_LOG("* limits:");
        VkPhysicalDeviceLimits& limits = deviceProperties.limits;
        RS_LOG(" - framebufferColorSampleCounts:%d, framebufferDepthSampleCounts:%d, framebufferStencilSampleCounts:%d, framebufferNoAttachmentsSampleCounts:%d", 
            limits.framebufferColorSampleCounts, limits.framebufferDepthSampleCounts, limits.framebufferStencilSampleCounts,
            limits.framebufferNoAttachmentsSampleCounts);
	    RS_LOG(" - maxComputeWorkGroupCount:[%d,%d,%d], maxComputeWorkGroupInvocations:%d, maxComputeWorkGroupSize:[%d,%d,%d] ",
		    limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2],
		    limits.maxComputeWorkGroupInvocations, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], 
            limits.maxComputeWorkGroupSize[2]);
        RS_LOG(" - maxBoundDescriptorSets:               %d ", limits.maxBoundDescriptorSets);
        RS_LOG(" - maxColorAttachments:                  %d ", limits.maxColorAttachments);
        RS_LOG(" - maxComputeSharedMemorySize:           %d ", limits.maxComputeSharedMemorySize);
        RS_LOG(" - maxDescriptorSetInputAttachments:     %d ", limits.maxDescriptorSetInputAttachments);
        RS_LOG(" - maxDescriptorSetSampledImages:        %d ", limits.maxDescriptorSetSampledImages);
        RS_LOG(" - maxDescriptorSetSamplers:             %d ", limits.maxDescriptorSetSamplers);
        RS_LOG(" - maxDescriptorSetStorageBuffers:       %d ", limits.maxDescriptorSetStorageBuffers);
        RS_LOG(" - maxDescriptorSetStorageBuffersDynamic:%d ", limits.maxDescriptorSetStorageBuffersDynamic);
        RS_LOG(" - maxDrawIndexedIndexValue:             %u ", limits.maxDrawIndexedIndexValue);
        RS_LOG(" - maxDrawIndirectCount:                 %u ", limits.maxDrawIndirectCount);
        RS_LOG(" - maxFragmentCombinedOutputResources:   %d ", limits.maxFragmentCombinedOutputResources);
        RS_LOG(" - maxFragmentInputComponents:           %d ", limits.maxFragmentInputComponents);
        RS_LOG(" - maxFragmentOutputAttachments:         %d ", limits.maxFragmentOutputAttachments);
        RS_LOG(" - maxFramebufferHeight:                 %d ", limits.maxFramebufferHeight);
        RS_LOG(" - maxFramebufferWidth:                  %d ", limits.maxFramebufferWidth);
        RS_LOG(" - maxFramebufferLayers:                 %d ", limits.maxFramebufferLayers);
        RS_LOG(" - maxImageArrayLayers:                  %d ", limits.maxImageArrayLayers);
        RS_LOG(" - maxImageDimension1D:                  %d ", limits.maxImageDimension1D);
        RS_LOG(" - maxImageDimension2D:                  %d ", limits.maxImageDimension2D);
        RS_LOG(" - maxImageDimension3D:                  %d ", limits.maxImageDimension3D);
        RS_LOG(" - maxImageDimensionCube:                %d ", limits.maxImageDimensionCube);
        RS_LOG(" - maxInterpolationOffset:               %f ", limits.maxInterpolationOffset);
        RS_LOG(" - maxMemoryAllocationCount:             %d ", limits.maxMemoryAllocationCount);
        RS_LOG(" - maxSampleMaskWords:                   %d ", limits.maxSampleMaskWords);
        RS_LOG(" - maxPerStageDescriptorInputAttachments:%d ", limits.maxPerStageDescriptorInputAttachments);
        RS_LOG(" - maxPerStageDescriptorSampledImages:   %d ", limits.maxPerStageDescriptorSampledImages);
        RS_LOG(" - maxPerStageDescriptorSamplers:        %d ", limits.maxPerStageDescriptorSamplers);
        RS_LOG(" - maxPerStageDescriptorUniformBuffers:  %d ", limits.maxPerStageDescriptorUniformBuffers);
        RS_LOG(" - maxPerStageDescriptorStorageBuffers:  %d ", limits.maxPerStageDescriptorStorageBuffers);
        RS_LOG(" - maxPerStageDescriptorStorageImages:   %d ", limits.maxPerStageDescriptorStorageImages);
        RS_LOG(" - maxPerStageResources:                 %d ", limits.maxPerStageResources);
        RS_LOG(" - maxPushConstantsSize:                 %d ", limits.maxPushConstantsSize);
        RS_LOG(" - maxSamplerAllocationCount:            %d ", limits.maxSamplerAllocationCount);
        RS_LOG(" - maxSamplerAnisotropy:                 %f ", limits.maxSamplerAnisotropy);
        RS_LOG(" - maxSamplerLodBias:                    %f ", limits.maxSamplerLodBias);
        RS_LOG(" - maxStorageBufferRange:                %d ", limits.maxStorageBufferRange);
        RS_LOG(" - maxTexelBufferElements:               %d ", limits.maxTexelBufferElements);
        RS_LOG(" - minTexelGatherOffset:                 %d ", limits.minTexelGatherOffset);
        RS_LOG(" - maxTexelGatherOffset:                 %d ", limits.maxTexelGatherOffset);
        RS_LOG(" - maxTexelOffset:                       %d ", limits.maxTexelOffset);
        RS_LOG(" - minInterpolationOffset:               %f ", limits.minInterpolationOffset);
        RS_LOG(" - maxVertexInputAttributes:             %d ", limits.maxVertexInputAttributes);
        RS_LOG(" - minUniformBufferOffsetAlignment:      %d ", limits.minUniformBufferOffsetAlignment);
        RS_LOG(" - mipmapPrecisionBits:                  %d ", limits.mipmapPrecisionBits);
        RS_LOG(" - storageImageSampleCounts:             %d ", (int)limits.storageImageSampleCounts);

        /*
         uint32_t        memoryTypeCount;
         VkMemoryType    memoryTypes[VK_MAX_MEMORY_TYPES];
         uint32_t        memoryHeapCount;
         VkMemoryHeap    [VK_MAX_MEMORY_HEAPS];

         VkMemoryPropertyFlags    propertyFlags;
        uint32_t                 heapIndex;

        typedef struct  {
            VkDeviceSize         size;
            VkMemoryHeapFlags    flags;
        } VkMemoryHeap;

        */



        /*
        void*& next = devicePropes->pNext;
        if(GVulkanExtension(VK_KHR_get_physical_device_properties2).enable){
            deviceProperties2.pNext = next;
            next = &deviceProperties2;
            if(isDeviceIDPropertiesEnabled()){
                deviceID.pNext = next;
                next = &deviceID;
            }
            getPhysicalDeviceProperties2KHR(&deviceProperties2);
        }
        if(GVulkanExtension(VK_KHR_fragment_shading_rate).enable){
        }
        */
        auto getQueueFamilyProp = [](VkPhysicalDevice handle, uint32& count, VkQueueFamilyProperties* data){
            GInstanceCommands()->vkGetPhysicalDeviceQueueFamilyProperties(handle, &count, data);
            return VK_SUCCESS;
        };
        enumerateProperties(queueFamilyProperties, getQueueFamilyProp, physicalHandle);
        RS_LOG("familyIndex:%d, handle:%p", queues[1].familyIndex, queues[1].handle);
        rs_check(queues[1].familyIndex == kInvalidInteger<uint32>);
        rs_check(queues[1].handle == VK_NULL_HANDLE);
    }


    void getQueueFamily(std::vector<WDeviceQueueCreateInfo>& queueInfos, std::vector<float>& priorities){
        uint32 totalCount =0;
        bool allowAsyncCompute = false;
        for(uint32 i=0; i<queueFamilyProperties.size(); ++i){
            auto&& prop = queueFamilyProperties[i];
            RS_LOG("** queue %d", i );
            RS_LOG("** queueFlags %d", prop.queueFlags );
            RS_LOG("** queueCount %d", prop.queueCount );
            RS_LOG("** timestampValidBits %d", prop.timestampValidBits );
            RS_LOG("** minImageTransferGranularity %d", prop.minImageTransferGranularity.width,
             prop.minImageTransferGranularity.height, prop.minImageTransferGranularity.depth );

            bool foundIndex = false;
            auto foundQueueFamilyIndex = [&](VkQueueFlagBits flag, EVkQueueType type){
                if ((prop.queueFlags & flag) == flag){
                    if(getQueue(type).familyIndex == kInvalidInteger<uint32>){
                        getQueue(type).familyIndex = i;
                        foundIndex=true;
                    }
                }
            };
            foundQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, EVkQueueType::Graphics);
            foundQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, EVkQueueType::Compute);
            foundQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, EVkQueueType::Transfer);
            foundQueueFamilyIndex(VK_QUEUE_SPARSE_BINDING_BIT, EVkQueueType::Sparse);

            if(!foundIndex){
                RS_LOG("skip queue family because it's not used");
                continue;
            }

            WDeviceQueueCreateInfo queueInfo;
            queueInfo.queueFamilyIndex = i;
            queueInfo.queueCount = prop.queueCount;
            totalCount += prop.queueCount;
            queueInfos.emplace_back(queueInfo);
        }
        priorities.resize(totalCount);
        float* data = priorities.data();
        for(uint32 i=0; i<queueFamilyProperties.size(); ++i){
            auto&& prop = queueFamilyProperties[i];
            auto&& queueInfo = queueInfos[i];
            queueInfo.pQueuePriorities = data;
            for(uint32 i=0; i<prop.queueCount;++i ){
                *(data+i) = 1.0f;
            }
            data+=prop.queueCount;
        }
    }

    bool createQueue(){
        if(getQueue(EVkQueueType::Graphics).familyIndex==kInvalidInteger<uint32>){
            RS_LOG("invalid graphics queue index");
            return false;
        }
        getDeviceQueue(getQueue(EVkQueueType::Graphics).familyIndex, 0, &getQueue(EVkQueueType::Graphics).handle);
        auto optionalQueue = [&](EVkQueueType type){
            if(getQueue(type).familyIndex == kInvalidInteger<uint32>){
                getQueue(type).familyIndex = getQueue(EVkQueueType::Graphics).familyIndex;
                getQueue(type).index = 0;
                getDeviceQueue(getQueue(type).familyIndex, 0, &getQueue(type).handle);
                rs_check(getQueue(type).handle!=VK_NULL_HANDLE);
            }
        };
        optionalQueue(EVkQueueType::Compute);
        optionalQueue(EVkQueueType::Transfer);
        optionalQueue(EVkQueueType::Sparse);
        return true;
    }
};

struct VulkanSemaphore{
    VkSemaphore handle;
    VulkanDevice& device;

    VulkanSemaphore(VulkanDevice& inDevice)
        : handle{VK_NULL_HANDLE}
        , device{inDevice}
    {
        WSemaphoreCreateInfo info;
        device.createSemaphore(&info, &handle);
        rs_check(handle!=VK_NULL_HANDLE);
    }

    ~VulkanSemaphore(){
        device.destroySemaphore(handle);
    }
};

/**
 * @brief 
 * 
 * @tparam VulkanContext should implements initBasicEntryPoints
 */
template<typename Derive>
struct VulkanContext
{
    using Type = Derive;
    VulkanInstance instance;
    std::vector<VulkanDevice> devices;
    
    // debug
    VkDebugReportCallbackEXT MsgCallback = VK_NULL_HANDLE;

    void addExtensionIfExists(std::vector<const char*>& outExtensions,
        std::vector<std::string>& uniqueExtensions,
        std::vector<const char*>& extensionRequires){
        for(auto&& require: extensionRequires){
            for(auto&& existExtension : uniqueExtensions){
                if(strcmp(require, existExtension.c_str()) == 0){
                    outExtensions.emplace_back(existExtension.c_str());
                    continue;
                }
            }
        }
    }

    template<typename GetLayer, typename GetExtension>
    void getLayerAndExtension(
        std::vector<const char*>& outExtensions, 
        std::vector<const char*>& outLayers,
        std::vector<VulkanLayerExtensions>& layerAndExtensions,
        std::vector<std::string>& uniqueExtensions,
        std::vector<const char*> requireExtensions,
        GetLayer&& getLayer, GetExtension&& getExtension){

        uniqueExtensions.clear();
        layerAndExtensions.clear();
        layerAndExtensions.resize(1);

        // layers
        std::vector<VkLayerProperties> layers;
        enumerateProperties( layers, getLayer, layerAndExtensions );
        RS_LOG("** VkLayer count:%d %d*****",layerAndExtensions.size(),layers.size());

        // extensions
        for(uint32 i=0; i<layerAndExtensions.size(); ++i){
            auto&& layerExtensions = layerAndExtensions[i];
            auto&& layer = layerExtensions.layer;
            auto&& extensions = layerExtensions.extensions;
            enumerateProperties( extensions, getExtension , i==0 ? nullptr : layer.layerName );
            RS_LOG("** VkLayer[%d]:%s version:%d, extension count:%d", i, layer.layerName, layer.specVersion, extensions.size());
            for(uint32 j=0; j<extensions.size(); ++j){
                RS_LOG("** VkLayer:%s extension[%d]:%s", layer.layerName, j, extensions[j].extensionName);
                //if(strcmp("VK_LAYER_KHRONOS_validation", layer.layerName))
                {
                    uniqueExtensions.emplace_back(extensions[j].extensionName);
                }
            }
            if(strlen(layer.layerName)>0){
                //if(strcmp("VK_LAYER_KHRONOS_validation", layer.layerName))
                {
                    outLayers.emplace_back(layer.layerName);
                }
            }
            
        }
        auto&& last = std::unique(uniqueExtensions.begin(), uniqueExtensions.end());
        uniqueExtensions.erase(last, uniqueExtensions.end());

        auto&& removeIt = std::unique(outLayers.begin(), outLayers.end(), [](char const* str1, char const* str2){return 0 == strcmp(str1,str2);});
        outLayers.erase(removeIt, outLayers.end());


        for(auto&& extension : uniqueExtensions){
            RS_LOG("-- unique extension: %s", extension.c_str());
        }
        addExtensionIfExists( outExtensions, uniqueExtensions, requireExtensions );
        auto&& finalLast = std::unique(outExtensions.begin(), outExtensions.end());
        outExtensions.erase(finalLast, outExtensions.end());

        RS_LOG("layer count:%d", outLayers.size());
        for(auto&& layer : outLayers){
            RS_LOG("** final layer: %s", layer);
        }

        RS_LOG("extension count:%d", outExtensions.size());
        for(auto&& extension : outExtensions){
            RS_LOG("** final extension: %s", extension);
        }
    }

    bool createInstance(){
        // enumerate the extension
        auto getInstanceExtensionProperties = [](const char* name, uint32& count, VkExtensionProperties* data){
            return GGlobalCommands()->vkEnumerateInstanceExtensionProperties(name, &count, data);
        };

        // enumerate the layer
        auto getInstanceLayerProperties = [](std::vector<VulkanLayerExtensions>& layers, uint32& count, VkLayerProperties* data ){
            VkResult ret = GGlobalCommands()->vkEnumerateInstanceLayerProperties(&count, data);
            if(data!=nullptr){
                layers.resize(count+1);
                for(uint32 i=0; i<count; ++i){
                    layers[i+1].layer = *data;
                }
            }
            return ret;
        };
        
        std::vector<const char*> outInstanceExtension{};
        std::vector<const char*> outInstanceLayers{};
        std::vector<const char*> availableExtensions{};
        GetVulkanInstanceExtensionStrings(availableExtensions);

        RS_LOG("*** Instance Layer and Extension: **");
        getLayerAndExtension( outInstanceExtension, outInstanceLayers, 
             instance.layerExtensions, instance.uniqueExtensions, availableExtensions, 
             getInstanceLayerProperties, getInstanceExtensionProperties);

        WApplicationInfo appInfo;
        appInfo.pNext= nullptr;
        appInfo.pApplicationName= "RSDebugLayerPlayer";
        appInfo.applicationVersion= 1;
        appInfo.pEngineName= "TestEngine";
        appInfo.engineVersion=1;
        appInfo.apiVersion=VK_API_VERSION_1_0;
        WInstanceCreateInfo CreateInfo;
        CreateInfo.pApplicationInfo = &appInfo;
        CreateInfo.setEnabledLayerNames(outInstanceLayers);
        CreateInfo.setEnabledExtensionNames(outInstanceExtension);
        rs_check(GGlobalCommands()->vkCreateInstance!=nullptr);
        VkResult ret = GGlobalCommands()->vkCreateInstance(&CreateInfo, nullptr, &instance.handle);

        if (ret != VK_SUCCESS){
            RS_LOG("create instance raise %s error, exit!", GetVkResultString(ret));
            return false;
        }

        // get instance proc addr
        if(!instance.GetCommandAddrs()){
            RS_LOG("get instance commands  error, exit!");
            return false;
        }
        // set to global
        GInstanceCommandsRef() = &instance;
        return true;
    }
    
    bool createDevice(){
        RS_LOG("** createDevice ");
        std::vector<VkPhysicalDevice> physicalDevices;
        // enumerate physical device
        auto getPhysicalDevice = [](uint32& count, VkPhysicalDevice* data){
            rs_check(GInstanceCommands()->vkEnumeratePhysicalDevices!=nullptr);
            return GInstanceCommands()->vkEnumeratePhysicalDevices(GInstanceCommands()->handle, &count, data);
        };
        enumerateProperties(physicalDevices, getPhysicalDevice);
        RS_LOG("** physical device count : %d", physicalDevices.size());
        if(physicalDevices.size() == 0){
            RS_LOG(" no physical found, exit!");
            return false;
        }
        devices.resize(physicalDevices.size());

        std::vector<const char*> outDeviceExtensions{};
        std::vector<const char*> outDeviceLayers{};
        std::vector<const char*> availableExtensions{};
        GetVulkanDeviceExtensionStrings(availableExtensions);

        // for each device
        for(uint32 i=0; i<physicalDevices.size(); ++i){
            auto&& physicalDevice = physicalDevices[i];
            auto&& device =  devices[i];
            device.physicalHandle = physicalDevice;

            // enumerate the extension
            auto getDeviceExtensionProperties = [&physicalDevice](const char* name, uint32& count, VkExtensionProperties* data){
                rs_check(GInstanceCommands()->vkEnumerateDeviceExtensionProperties!=nullptr);
                return GInstanceCommands()->vkEnumerateDeviceExtensionProperties(physicalDevice, name, &count, data);
            };

            // enumerate the layer
            auto getDeviceLayerProperties = [&physicalDevice](std::vector<VulkanLayerExtensions>& layers, uint32& count, VkLayerProperties* data ){
                rs_check(GInstanceCommands()->vkEnumerateDeviceLayerProperties!=nullptr);
                VkResult ret = GInstanceCommands()->vkEnumerateDeviceLayerProperties(physicalDevice, &count, data);
                if(data!=nullptr){
                    layers.resize(count+1);
                    for(uint32 i=0; i<count; ++i){
                        layers[i+1].layer = *data;
                    }
                }
                return ret;
            };

            RS_LOG("*** Device Layer and Extension: **");
            getLayerAndExtension( outDeviceExtensions, outDeviceLayers, device.layerExtensions,
                device.uniqueExtensions, availableExtensions, 
                getDeviceLayerProperties, getDeviceExtensionProperties);

            

            // query gpu properties
            device.queryGPUProperties(i);

            // create device
            std::vector<WDeviceQueueCreateInfo> queueInfos;
            std::vector<float> priorities;
            device.getQueueFamily(queueInfos, priorities);
       
            WDeviceCreateInfo createInfo;
            createInfo.setEnabledLayerNames(outDeviceLayers);
            createInfo.setEnabledExtensionNames(outDeviceExtensions);
            createInfo.setQueueCreateInfos(queueInfos);
            WPhysicalDeviceFeatures deviceFeatures{};
            createInfo.pEnabledFeatures = &deviceFeatures;

            // create device handle
            VkDevice deviceHandle = VK_NULL_HANDLE;
            VkResult ret = device.createDevice(&createInfo, &deviceHandle);
            device.handle = deviceHandle;
            if (ret != VK_SUCCESS){
                RS_LOG("create device raise %s error, exit!", GetVkResultString(ret));
                return false;
            }

            // get instance proc addr
            if(!device.GetCommandAddrs()){
                RS_LOG("get instance commands  error, exit!");
                return false;
            }
            // set to global
            GDeviceCommandsRef(i) = &device;

            // get queues
            if(!device.createQueue()){
                RS_LOG("create queue  error, exit!");
                return false;
            }
        }
        return true;
    }

    bool initBasicEntryPoints(){ return false; }

    bool initialize(){
        RS_LOG("vulkan context: initialize");
        Derive* derive = static_cast<Derive*>(this);
        if(! derive->initBasicEntryPoints()){
            return false;
        }
        RS_LOG("vulkan context: initBasicEntryPoints succ");

        if(!createInstance()){
            return false;
        }
        RS_LOG("vulkan context: createInstance succ");

        setupDebugLayerCallback();
        
        if(!createDevice()){
            return false;
        }
        RS_LOG("vulkan context: createDevice succ");
        return true;
    }

    bool finalize(){
        RS_LOG("vulkan context: finalize");
        if(instance.handle!=VK_NULL_HANDLE){
            GInstanceCommands()->vkDestroyInstance(instance.handle, nullptr);
            instance.handle = VK_NULL_HANDLE;
        }
        

        RS_LOG("vulkan context: destroy instance");
        return true;
    }

    void setupDebugLayerCallback()
    {
        /*
        if (IsVkExtensionEnabled(VULKAN_SUPPORTS_DEBUG_UTILS))
        {
            PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)(void*)VulkanRHI::vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
            if (CreateDebugUtilsMessengerEXT)
            {
                VkDebugUtilsMessengerCreateInfoEXT CreateInfo;
                ZeroVulkanStruct(CreateInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);

                int32 CVar = GValidationCvar.GetValueOnRenderThread();
                CreateInfo.messageSeverity = (CVar >= 1 ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : 0) |
                    (CVar >= 2 ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : 0) | (CVar >= 3 ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT : 0);
                CreateInfo.messageType = (CVar >= 1 ? (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) : 0) |
                    (CVar >= 3 ? VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT : 0);
                CreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)(void*)DebugUtilsCallback;
                VkResult Result = (*CreateDebugUtilsMessengerEXT)(Instance, &CreateInfo, nullptr, &Messenger);
                ensure(Result == VK_SUCCESS);
            }
        }
        else */

        RS_LOG("VK_EXT_debug_report:%d", (int)IsVkExtensionEnabled(VK_EXT_debug_report));
        if (IsVkExtensionEnabled(VK_EXT_debug_report))
        {
            rs_check(GInstanceCommands()->vkCreateDebugReportCallbackEXT!=nullptr);
            {
                WDebugReportCallbackCreateInfoEXT CreateInfo;
                CreateInfo.pfnCallback = DebugReportFunction;
                //CreateInfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
                //CreateInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
                //CreateInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
                //CreateInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
                CreateInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
                VkResult ret = GInstanceCommands()->vkCreateDebugReportCallbackEXT(GInstanceCommands()->handle, &CreateInfo, nullptr, &MsgCallback);
                if(ret!=VK_SUCCESS){
                    RS_LOG_FILE(" vkCreateDebugReportCallbackEXT fail:%s \n", GetVkResultString(ret));
                }

                RS_LOG("#enable debug report extension");
            }   
        }
    }

};

PROJECT_NAMESPACE_END