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


    AR_LOG_FILE("*** [%s:%s:%d] Obj 0x%p Loc %d %s\n", MsgPrefix, LayerPrefix, MsgCode, (void*)SrcObject, (uint32)Location, Msg);

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

        AR_LOG("** GPU info **");
        AR_LOG("* api version:%d", deviceProperties.apiVersion );
        AR_LOG("* driver version:%d", deviceProperties.driverVersion );
        AR_LOG("* vendorID:%d", deviceProperties.vendorID );
        AR_LOG("* deviceID:%d", deviceProperties.deviceID );
        AR_LOG("* deviceType:%d", deviceProperties.deviceType );
        AR_LOG("* deviceName:%s", deviceProperties.deviceName );
        AR_LOG("* pipelineCacheUUID:%s", deviceProperties.pipelineCacheUUID );
        AR_LOG("* limits:");
        VkPhysicalDeviceLimits& limits = deviceProperties.limits;
        AR_LOG(" - framebufferColorSampleCounts:%d, framebufferDepthSampleCounts:%d, framebufferStencilSampleCounts:%d, framebufferNoAttachmentsSampleCounts:%d", 
            limits.framebufferColorSampleCounts, limits.framebufferDepthSampleCounts, limits.framebufferStencilSampleCounts,
            limits.framebufferNoAttachmentsSampleCounts);
	    AR_LOG(" - maxComputeWorkGroupCount:[%d,%d,%d], maxComputeWorkGroupInvocations:%d, maxComputeWorkGroupSize:[%d,%d,%d] ",
		    limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2],
		    limits.maxComputeWorkGroupInvocations, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], 
            limits.maxComputeWorkGroupSize[2]);
        AR_LOG(" - maxBoundDescriptorSets:               %d ", limits.maxBoundDescriptorSets);
        AR_LOG(" - maxColorAttachments:                  %d ", limits.maxColorAttachments);
        AR_LOG(" - maxComputeSharedMemorySize:           %d ", limits.maxComputeSharedMemorySize);
        AR_LOG(" - maxDescriptorSetInputAttachments:     %d ", limits.maxDescriptorSetInputAttachments);
        AR_LOG(" - maxDescriptorSetSampledImages:        %d ", limits.maxDescriptorSetSampledImages);
        AR_LOG(" - maxDescriptorSetSamplers:             %d ", limits.maxDescriptorSetSamplers);
        AR_LOG(" - maxDescriptorSetStorageBuffers:       %d ", limits.maxDescriptorSetStorageBuffers);
        AR_LOG(" - maxDescriptorSetStorageBuffersDynamic:%d ", limits.maxDescriptorSetStorageBuffersDynamic);
        AR_LOG(" - maxDrawIndexedIndexValue:             %u ", limits.maxDrawIndexedIndexValue);
        AR_LOG(" - maxDrawIndirectCount:                 %u ", limits.maxDrawIndirectCount);
        AR_LOG(" - maxFragmentCombinedOutputResources:   %d ", limits.maxFragmentCombinedOutputResources);
        AR_LOG(" - maxFragmentInputComponents:           %d ", limits.maxFragmentInputComponents);
        AR_LOG(" - maxFragmentOutputAttachments:         %d ", limits.maxFragmentOutputAttachments);
        AR_LOG(" - maxFramebufferHeight:                 %d ", limits.maxFramebufferHeight);
        AR_LOG(" - maxFramebufferWidth:                  %d ", limits.maxFramebufferWidth);
        AR_LOG(" - maxFramebufferLayers:                 %d ", limits.maxFramebufferLayers);
        AR_LOG(" - maxImageArrayLayers:                  %d ", limits.maxImageArrayLayers);
        AR_LOG(" - maxImageDimension1D:                  %d ", limits.maxImageDimension1D);
        AR_LOG(" - maxImageDimension2D:                  %d ", limits.maxImageDimension2D);
        AR_LOG(" - maxImageDimension3D:                  %d ", limits.maxImageDimension3D);
        AR_LOG(" - maxImageDimensionCube:                %d ", limits.maxImageDimensionCube);
        AR_LOG(" - maxInterpolationOffset:               %f ", limits.maxInterpolationOffset);
        AR_LOG(" - maxMemoryAllocationCount:             %d ", limits.maxMemoryAllocationCount);
        AR_LOG(" - maxSampleMaskWords:                   %d ", limits.maxSampleMaskWords);
        AR_LOG(" - maxPerStageDescriptorInputAttachments:%d ", limits.maxPerStageDescriptorInputAttachments);
        AR_LOG(" - maxPerStageDescriptorSampledImages:   %d ", limits.maxPerStageDescriptorSampledImages);
        AR_LOG(" - maxPerStageDescriptorSamplers:        %d ", limits.maxPerStageDescriptorSamplers);
        AR_LOG(" - maxPerStageDescriptorUniformBuffers:  %d ", limits.maxPerStageDescriptorUniformBuffers);
        AR_LOG(" - maxPerStageDescriptorStorageBuffers:  %d ", limits.maxPerStageDescriptorStorageBuffers);
        AR_LOG(" - maxPerStageDescriptorStorageImages:   %d ", limits.maxPerStageDescriptorStorageImages);
        AR_LOG(" - maxPerStageResources:                 %d ", limits.maxPerStageResources);
        AR_LOG(" - maxPushConstantsSize:                 %d ", limits.maxPushConstantsSize);
        AR_LOG(" - maxSamplerAllocationCount:            %d ", limits.maxSamplerAllocationCount);
        AR_LOG(" - maxSamplerAnisotropy:                 %f ", limits.maxSamplerAnisotropy);
        AR_LOG(" - maxSamplerLodBias:                    %f ", limits.maxSamplerLodBias);
        AR_LOG(" - maxStorageBufferRange:                %d ", limits.maxStorageBufferRange);
        AR_LOG(" - maxTexelBufferElements:               %d ", limits.maxTexelBufferElements);
        AR_LOG(" - minTexelGatherOffset:                 %d ", limits.minTexelGatherOffset);
        AR_LOG(" - maxTexelGatherOffset:                 %d ", limits.maxTexelGatherOffset);
        AR_LOG(" - maxTexelOffset:                       %d ", limits.maxTexelOffset);
        AR_LOG(" - minInterpolationOffset:               %f ", limits.minInterpolationOffset);
        AR_LOG(" - maxVertexInputAttributes:             %d ", limits.maxVertexInputAttributes);
        AR_LOG(" - minUniformBufferOffsetAlignment:      %d ", limits.minUniformBufferOffsetAlignment);
        AR_LOG(" - mipmapPrecisionBits:                  %d ", limits.mipmapPrecisionBits);
        AR_LOG(" - storageImageSampleCounts:             %d ", (int)limits.storageImageSampleCounts);

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
        AR_LOG("familyIndex:%d, handle:%p", queues[1].familyIndex, queues[1].handle);
        rs_check(queues[1].familyIndex == kInvalidInteger<uint32>);
        rs_check(queues[1].handle == VK_NULL_HANDLE);
    }


    void getQueueFamily(std::vector<WDeviceQueueCreateInfo>& queueInfos, std::vector<float>& priorities){
        uint32 totalCount =0;
        bool allowAsyncCompute = false;
        for(uint32 i=0; i<queueFamilyProperties.size(); ++i){
            auto&& prop = queueFamilyProperties[i];
            AR_LOG("** queue %d", i );
            AR_LOG("** queueFlags %d", prop.queueFlags );
            AR_LOG("** queueCount %d", prop.queueCount );
            AR_LOG("** timestampValidBits %d", prop.timestampValidBits );
            AR_LOG("** minImageTransferGranularity %d", prop.minImageTransferGranularity.width,
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
                AR_LOG("skip queue family because it's not used");
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
            AR_LOG("invalid graphics queue index");
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
        AR_LOG("** VkLayer count:%d %d*****",layerAndExtensions.size(),layers.size());

        // extensions
        for(uint32 i=0; i<layerAndExtensions.size(); ++i){
            auto&& layerExtensions = layerAndExtensions[i];
            auto&& layer = layerExtensions.layer;
            auto&& extensions = layerExtensions.extensions;
            enumerateProperties( extensions, getExtension , i==0 ? nullptr : layer.layerName );
            AR_LOG("** VkLayer[%d]:%s version:%d, extension count:%d", i, layer.layerName, layer.specVersion, extensions.size());
            for(uint32 j=0; j<extensions.size(); ++j){
                AR_LOG("** VkLayer:%s extension[%d]:%s", layer.layerName, j, extensions[j].extensionName);
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
            AR_LOG("-- unique extension: %s", extension.c_str());
        }
        addExtensionIfExists( outExtensions, uniqueExtensions, requireExtensions );
        auto&& finalLast = std::unique(outExtensions.begin(), outExtensions.end());
        outExtensions.erase(finalLast, outExtensions.end());

        AR_LOG("layer count:%d", outLayers.size());
        for(auto&& layer : outLayers){
            AR_LOG("** final layer: %s", layer);
        }

        AR_LOG("extension count:%d", outExtensions.size());
        for(auto&& extension : outExtensions){
            AR_LOG("** final extension: %s", extension);
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

        AR_LOG("*** Instance Layer and Extension: **");
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
            AR_LOG("create instance raise %s error, exit!", GetVkResultString(ret));
            return false;
        }

        // get instance proc addr
        if(!instance.GetCommandAddrs()){
            AR_LOG("get instance commands  error, exit!");
            return false;
        }
        // set to global
        GInstanceCommandsRef() = &instance;
        return true;
    }
    
    bool createDevice(){
        AR_LOG("** createDevice ");
        std::vector<VkPhysicalDevice> physicalDevices;
        // enumerate physical device
        auto getPhysicalDevice = [](uint32& count, VkPhysicalDevice* data){
            rs_check(GInstanceCommands()->vkEnumeratePhysicalDevices!=nullptr);
            return GInstanceCommands()->vkEnumeratePhysicalDevices(GInstanceCommands()->handle, &count, data);
        };
        enumerateProperties(physicalDevices, getPhysicalDevice);
        AR_LOG("** physical device count : %d", physicalDevices.size());
        if(physicalDevices.size() == 0){
            AR_LOG(" no physical found, exit!");
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

            AR_LOG("*** Device Layer and Extension: **");
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
                AR_LOG("create device raise %s error, exit!", GetVkResultString(ret));
                return false;
            }

            // get instance proc addr
            if(!device.GetCommandAddrs()){
                AR_LOG("get instance commands  error, exit!");
                return false;
            }
            // set to global
            GDeviceCommandsRef(i) = &device;

            // get queues
            if(!device.createQueue()){
                AR_LOG("create queue  error, exit!");
                return false;
            }
        }
        return true;
    }

    bool initBasicEntryPoints(){ return false; }

    bool initialize(){
        AR_LOG("vulkan context: initialize");
        Derive* derive = static_cast<Derive*>(this);
        if(! derive->initBasicEntryPoints()){
            return false;
        }
        AR_LOG("vulkan context: initBasicEntryPoints succ");

        if(!createInstance()){
            return false;
        }
        AR_LOG("vulkan context: createInstance succ");

        setupDebugLayerCallback();
        
        if(!createDevice()){
            return false;
        }
        AR_LOG("vulkan context: createDevice succ");
        return true;
    }

    bool finalize(){
        AR_LOG("vulkan context: finalize");
        if(instance.handle!=VK_NULL_HANDLE){
            GInstanceCommands()->vkDestroyInstance(instance.handle, nullptr);
            instance.handle = VK_NULL_HANDLE;
        }
        

        AR_LOG("vulkan context: destroy instance");
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

        AR_LOG("VK_EXT_debug_report:%d", (int)IsVkExtensionEnabled(VK_EXT_debug_report));
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
                    AR_LOG_FILE(" vkCreateDebugReportCallbackEXT fail:%s \n", GetVkResultString(ret));
                }

                AR_LOG("#enable debug report extension");
            }   
        }
    }

};

PROJECT_NAMESPACE_END