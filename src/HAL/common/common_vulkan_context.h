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
		AR_LOG(Info, "invalid VkResult:%s at Expr:%s file:%s line:%d", GetVkResultString(code), expr, file, line);
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
            CheckVulkanResult(ret = fn(std::forward<Args>(args)..., count, &array));
        }

    }while(ret == VK_INCOMPLETE);
}


inline uint32 GetMipSize(VkFormat format, VkExtent3D extent, uint32 arrayIndex, uint32 mipIndex)
{
    ARCheck(mipIndex>=0 && mipIndex<=20);
    ARCheck(arrayIndex>=0);
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
    ARCheck(range.layerCount>0 && range.levelCount>0);
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
		ARCheck(false);
	}


    AR_LOG(Info, "*** [%s:%s:%d] Obj 0x%p Loc %d %s\n", MsgPrefix, LayerPrefix, MsgCode, (void*)SrcObject, (uint32)Location, Msg);

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
	AR_LOG(Info, "*** memory heap info ***");
	AR_LOG(Info, "** type count:%d, heap cout:%d", memoryProperties.memoryTypeCount, memoryProperties.memoryHeapCount);

	for (uint32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
		VkMemoryType& type = memoryProperties.memoryTypes[i];
		AR_LOG(Info, "-- memory type[%d], heap index:%d, property:%s", i, type.heapIndex, GetDeviceHeapFlagString(type.propertyFlags).c_str());
	}
	for (uint32 i = 0; i < memoryProperties.memoryHeapCount; ++i) {
		VkMemoryHeap& heap = memoryProperties.memoryHeaps[i];
		AR_LOG(Info, "-- heap type[%d], heap size:%lld(%f MB), flagsd:%d", i, heap.size, (double)heap.size / (1024.0*1024.0), heap.flags);
	}
}



constexpr VkLayerProperties kGlobalLayerProperties{};

// one layer may has multi-extension
struct VulkanOneLayerExtensions
{
    VkLayerProperties const* layerProperty{ &kGlobalLayerProperties };
    std::vector<VkExtensionProperties> extensions{};
};

inline static VulkanOneLayerExtensions kGloablLayerProperties{ };

inline bool LayerPropertiesEqual(VkLayerProperties const& p1, VkLayerProperties const& p2)
{
    return 0 == RawStrOps<char>::compare(p1.layerName, p2.layerName);
}

inline bool LayerPropertiesLess(VkLayerProperties const& p1, VkLayerProperties const& p2)
{
    return RawStrOps<char>::compare(p1.layerName, p2.layerName) < 0;
}

struct VulkanLayersAndExtensions {
    std::vector<VulkanOneLayerExtensions> layerExtensions;
    std::vector<VkLayerProperties> queriedLayerProperties;
    std::vector<const char*> avaiableLayers; // static define in header
    std::vector<const char*> uniqueLayers;   // queried unique layers
    std::vector<const char*> usedLayers;     // final used layers
    std::vector<const char*> avaiableExtensions; // static define in header
    std::vector<const char*> uniqueExtensions; // queried unique extensions
    std::vector<const char*> usedExtensions;   // final used extensions
};


// single instance
struct VulkanInstance: public WInstance {
    VulkanLayersAndExtensions layerAndExtensions;
    
    bool GetCommandAddrs(){
        #define GET_INSTANCE_PROC_ADDR(Func)  \
                this->Func = (PFN_##Func)GGlobalCommands()->vkGetInstanceProcAddr(handle, #Func); \
				AREnsureNoBreak(this->Func!=nullptr);                     
        ENUMERATE_VK_INSTANCE_API(GET_INSTANCE_PROC_ADDR);
        GET_INSTANCE_PROC_ADDR(vkGetDeviceProcAddr);
        #undef GET_INSTANCE_PROC_ADDR
        return true;
    }
};

#define ENABLE_DEVICE_ID_PROPERTIES_KHR ENABLE_VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES || ENABLE_VK_KHR_EXTERNAL_FENCE_CAPABILITIES || ENABLE_VK_KHR_EXTERNAL_MEMORY_CAPABILITIES


enum EVkQueueType:uint8{
    Graphics = 0x01,
    Compute = 0x02,
    Transfer = 0x04,
    Sparse = 0x0,
    Num
};


template<typename T>
struct VulkanParent
{
    using ParentType = VulkanParent<T>;
    T& parent;
    VulkanParent(T const& t) :parent(t) {}
};


//multi-gpu
struct VulkanDevice: public WDevice{
    static constexpr int32 kQueueCount = (int32)EVkQueueType::Num;
    uint32 deviceIndex;
    VulkanLayersAndExtensions       layerAndExtensions;
    WPhysicalDeviceProperties       deviceProperties;
    WPhysicalDeviceMemoryProperties memoryProperties;

    // queue properties
    std::vector<VulkanQueueFamily> queueFamilies;
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
        ARCheck(GInstanceCommands()->vkGetDeviceProcAddr);
        #define GET_DEVICE_PROC_ADDR(Func)  \
                this->Func = (PFN_##Func)GInstanceCommands()->vkGetDeviceProcAddr(handle, #Func); \
				AREnsure(this->Func!=nullptr);                     
        ENUMERATE_VK_DEVICE_API(GET_DEVICE_PROC_ADDR);
        #undef GET_DEVICE_PROC_ADDR
        return true;
    }

    void queryGPUProperties(uint32 index){
        ARCheck( physicalHandle!=VK_NULL_HANDLE);
        // query physical device properties
        getPhysicalDeviceProperties(&deviceProperties);
        getPhysicalDeviceMemoryProperties(&memoryProperties);
        DumpPhysicalMemoryProp(memoryProperties);

        AR_LOG(Info, "*** GPU info ***");
        AR_LOG(Info, "* api version:%d", deviceProperties.apiVersion );
        AR_LOG(Info, "* driver version:%d", deviceProperties.driverVersion );
        AR_LOG(Info, "* vendorID:%d", deviceProperties.vendorID );
        AR_LOG(Info, "* deviceID:%d", deviceProperties.deviceID );
        AR_LOG(Info, "* deviceType:%d", deviceProperties.deviceType );
        AR_LOG(Info, "* deviceName:%s", deviceProperties.deviceName );
        AR_LOG(Info, "* pipelineCacheUUID:%s", deviceProperties.pipelineCacheUUID );
        AR_LOG(Info, "* limits:");
        VkPhysicalDeviceLimits& limits = deviceProperties.limits;
        AR_LOG(Info, " - framebufferColorSampleCounts:%d, framebufferDepthSampleCounts:%d, framebufferStencilSampleCounts:%d, framebufferNoAttachmentsSampleCounts:%d", 
            limits.framebufferColorSampleCounts, limits.framebufferDepthSampleCounts, limits.framebufferStencilSampleCounts,
            limits.framebufferNoAttachmentsSampleCounts);
	    AR_LOG(Info, " - maxComputeWorkGroupCount:[%d,%d,%d], maxComputeWorkGroupInvocations:%d, maxComputeWorkGroupSize:[%d,%d,%d] ",
		    limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2],
		    limits.maxComputeWorkGroupInvocations, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], 
            limits.maxComputeWorkGroupSize[2]);
        AR_LOG(Info, " - maxBoundDescriptorSets:               %d ", limits.maxBoundDescriptorSets);
        AR_LOG(Info, " - maxColorAttachments:                  %d ", limits.maxColorAttachments);
        AR_LOG(Info, " - maxComputeSharedMemorySize:           %d ", limits.maxComputeSharedMemorySize);
        AR_LOG(Info, " - maxDescriptorSetInputAttachments:     %d ", limits.maxDescriptorSetInputAttachments);
        AR_LOG(Info, " - maxDescriptorSetSampledImages:        %d ", limits.maxDescriptorSetSampledImages);
        AR_LOG(Info, " - maxDescriptorSetSamplers:             %d ", limits.maxDescriptorSetSamplers);
        AR_LOG(Info, " - maxDescriptorSetStorageBuffers:       %d ", limits.maxDescriptorSetStorageBuffers);
        AR_LOG(Info, " - maxDescriptorSetStorageBuffersDynamic:%d ", limits.maxDescriptorSetStorageBuffersDynamic);
        AR_LOG(Info, " - maxDrawIndexedIndexValue:             %u ", limits.maxDrawIndexedIndexValue);
        AR_LOG(Info, " - maxDrawIndirectCount:                 %u ", limits.maxDrawIndirectCount);
        AR_LOG(Info, " - maxFragmentCombinedOutputResources:   %d ", limits.maxFragmentCombinedOutputResources);
        AR_LOG(Info, " - maxFragmentInputComponents:           %d ", limits.maxFragmentInputComponents);
        AR_LOG(Info, " - maxFragmentOutputAttachments:         %d ", limits.maxFragmentOutputAttachments);
        AR_LOG(Info, " - maxFramebufferHeight:                 %d ", limits.maxFramebufferHeight);
        AR_LOG(Info, " - maxFramebufferWidth:                  %d ", limits.maxFramebufferWidth);
        AR_LOG(Info, " - maxFramebufferLayers:                 %d ", limits.maxFramebufferLayers);
        AR_LOG(Info, " - maxImageArrayLayers:                  %d ", limits.maxImageArrayLayers);
        AR_LOG(Info, " - maxImageDimension1D:                  %d ", limits.maxImageDimension1D);
        AR_LOG(Info, " - maxImageDimension2D:                  %d ", limits.maxImageDimension2D);
        AR_LOG(Info, " - maxImageDimension3D:                  %d ", limits.maxImageDimension3D);
        AR_LOG(Info, " - maxImageDimensionCube:                %d ", limits.maxImageDimensionCube);
        AR_LOG(Info, " - maxInterpolationOffset:               %f ", limits.maxInterpolationOffset);
        AR_LOG(Info, " - maxMemoryAllocationCount:             %d ", limits.maxMemoryAllocationCount);
        AR_LOG(Info, " - maxSampleMaskWords:                   %d ", limits.maxSampleMaskWords);
        AR_LOG(Info, " - maxPerStageDescriptorInputAttachments:%d ", limits.maxPerStageDescriptorInputAttachments);
        AR_LOG(Info, " - maxPerStageDescriptorSampledImages:   %d ", limits.maxPerStageDescriptorSampledImages);
        AR_LOG(Info, " - maxPerStageDescriptorSamplers:        %d ", limits.maxPerStageDescriptorSamplers);
        AR_LOG(Info, " - maxPerStageDescriptorUniformBuffers:  %d ", limits.maxPerStageDescriptorUniformBuffers);
        AR_LOG(Info, " - maxPerStageDescriptorStorageBuffers:  %d ", limits.maxPerStageDescriptorStorageBuffers);
        AR_LOG(Info, " - maxPerStageDescriptorStorageImages:   %d ", limits.maxPerStageDescriptorStorageImages);
        AR_LOG(Info, " - maxPerStageResources:                 %d ", limits.maxPerStageResources);
        AR_LOG(Info, " - maxPushConstantsSize:                 %d ", limits.maxPushConstantsSize);
        AR_LOG(Info, " - maxSamplerAllocationCount:            %d ", limits.maxSamplerAllocationCount);
        AR_LOG(Info, " - maxSamplerAnisotropy:                 %f ", limits.maxSamplerAnisotropy);
        AR_LOG(Info, " - maxSamplerLodBias:                    %f ", limits.maxSamplerLodBias);
        AR_LOG(Info, " - maxStorageBufferRange:                %d ", limits.maxStorageBufferRange);
        AR_LOG(Info, " - maxTexelBufferElements:               %d ", limits.maxTexelBufferElements);
        AR_LOG(Info, " - minTexelGatherOffset:                 %d ", limits.minTexelGatherOffset);
        AR_LOG(Info, " - maxTexelGatherOffset:                 %d ", limits.maxTexelGatherOffset);
        AR_LOG(Info, " - maxTexelOffset:                       %d ", limits.maxTexelOffset);
        AR_LOG(Info, " - minInterpolationOffset:               %f ", limits.minInterpolationOffset);
        AR_LOG(Info, " - maxVertexInputAttributes:             %d ", limits.maxVertexInputAttributes);
        AR_LOG(Info, " - minUniformBufferOffsetAlignment:      %d ", limits.minUniformBufferOffsetAlignment);
        AR_LOG(Info, " - mipmapPrecisionBits:                  %d ", limits.mipmapPrecisionBits);
        AR_LOG(Info, " - storageImageSampleCounts:             %d ", (int)limits.storageImageSampleCounts);

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
        auto getQueueFamilyProp = [](VkPhysicalDevice handle, uint32& count, std::vector<VkQueueFamilyProperties>* allProperties){
            GInstanceCommands()->vkGetPhysicalDeviceQueueFamilyProperties(handle, &count, allProperties ? allProperties->data() : nullptr);
            return VK_SUCCESS;
        };
        enumerateProperties(queueFamilyProperties, getQueueFamilyProp, physicalHandle);
        AR_LOG(Info, "familyIndex:%d, handle:%p", queues[1].familyIndex, queues[1].handle);
        ARCheck(queues[1].familyIndex == kInvalidInteger<uint32>);
        ARCheck(queues[1].handle == VK_NULL_HANDLE);
    }


    void getQueueFamily(std::vector<WDeviceQueueCreateInfo>& queueInfos, std::vector<float>& priorities){
        uint32 totalCount =0;
        bool allowAsyncCompute = false;
        for(uint32 i=0; i<queueFamilyProperties.size(); ++i){
            auto&& prop = queueFamilyProperties[i];
            AR_LOG(Info, "** queue %d", i );
            AR_LOG(Info, "** queueFlags %d", prop.queueFlags );
            AR_LOG(Info, "** queueCount %d", prop.queueCount );
            AR_LOG(Info, "** timestampValidBits %d", prop.timestampValidBits );
            AR_LOG(Info, "** minImageTransferGranularity %d", prop.minImageTransferGranularity.width,
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
                AR_LOG(Info, "skip queue family because it's not used");
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
            AR_LOG(Info, "invalid graphics queue index");
            return false;
        }
        getDeviceQueue(getQueue(EVkQueueType::Graphics).familyIndex, 0, &getQueue(EVkQueueType::Graphics).handle);
        auto optionalQueue = [&](EVkQueueType type){
            if(getQueue(type).familyIndex == kInvalidInteger<uint32>){
                getQueue(type).familyIndex = getQueue(EVkQueueType::Graphics).familyIndex;
                getQueue(type).index = 0;
                getDeviceQueue(getQueue(type).familyIndex, 0, &getQueue(type).handle);
                ARCheck(getQueue(type).handle!=VK_NULL_HANDLE);
            }
        };
        optionalQueue(EVkQueueType::Compute);
        optionalQueue(EVkQueueType::Transfer);
        optionalQueue(EVkQueueType::Sparse);
        return true;
    }
};

struct VulkanQueueFamily : public WQueueFamilyProperties2
{
    i32 usedCount{ 0 };
    VulkanQueueFamily() : WQueueFamilyProperties2{} {
        MemoryOps::zero(this->queueFamilyProperties);
    }
    bool isFull() {
        return this->usedCount == this->queueFamilyProperties.queueCount;
    }
};

struct VulkanQueue {
    uint32 familyIndex{ kInvalidInteger<uint32> };
    uint32 index{ kInvalidInteger<uint32> };
    VkQueue handle{ VK_NULL_HANDLE };
};


struct VulkanSemaphore :public VulkanParent< VulkanDevice > {
    VkSemaphore handle;
    VulkanDevice& device;

    VulkanSemaphore(VulkanDevice& inDevice) 
        : ParentType{ inDevice }
        , handle{VK_NULL_HANDLE}
        , device{inDevice}
    {
        WSemaphoreCreateInfo info;
        device.createSemaphore(&info, &handle);
        ARCheck(handle!=VK_NULL_HANDLE);
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
struct CommonVulkanContext
{
    using Type = Derive;
    VulkanInstance instance;
    std::vector<VulkanDevice> devices;
    
    // debug
    VkDebugReportCallbackEXT MsgCallback = VK_NULL_HANDLE;

    void mergeSet(std::vector<const char*>& result,
        std::vector<const char*>& founds,
        std::vector<const char*>& availables)
    {
        for(auto&& available: availables){
            for(auto&& found : founds){
                if(strcmp(found, available) == 0){
                    result.emplace_back(available);
                    continue;
                }
            }
        }
    }

    template<typename GetLayer, typename GetExtension>
    void getLayerAndExtension( VulkanLayersAndExtensions& layersAndExtensions, GetLayer&& getLayer,  GetExtension&& getExtension){
        auto& uniqueExtensions = layersAndExtensions.uniqueExtensions;
        auto& uniqueLayers = layersAndExtensions.uniqueLayers;
        auto& layerExtensions = layersAndExtensions.layerExtensions;
        auto& queriedLayerProperties = layersAndExtensions.queriedLayerProperties;
        auto& finalUsedLayers = layersAndExtensions.usedLayers;
        auto& finalUsedExtensions = layersAndExtensions.usedExtensions;

        uniqueExtensions.clear();
        uniqueLayers.clear();
        layerExtensions.clear();
        layerExtensions.push_back(kGloablLayerProperties);

        // layers
        enumerateProperties(queriedLayerProperties, getLayer, layerExtensions);
        AR_LOG(Info, "** VkLayer count:%d %d **", layerExtensions.size(), queriedLayerProperties.size());

        // extensions
        for(uint32 i=0; i< layerExtensions.size(); ++i){
            auto&& oneLayerExtension = layerExtensions[i];
            auto&& layer = *oneLayerExtension.layerProperty;
            auto&& extensions = oneLayerExtension.extensions;
            enumerateProperties( extensions, getExtension , i==0 ? nullptr : layer.layerName );
            AR_LOG(Info, "** VkLayer[%d]:%s version:%d, extension count:%d ** ", i, layer.layerName[0]==0 ? "Global": layer.layerName, layer.specVersion, extensions.size());
            for(uint32 j=0; j<extensions.size(); ++j){
                AR_LOG(Info, "\t -- extension[%d]:%s", j, extensions[j].extensionName);
                if(extensions[j].extensionName[0]!=0)
                {
                    uniqueExtensions.emplace_back(extensions[j].extensionName);
                }
            }
            if(layer.layerName[0]!=0){
                uniqueLayers.emplace_back(layer.layerName);
            }
        }
        
        AlgOps::unique(uniqueExtensions, AlgOps::rawStrLess, AlgOps::rawStrEqual);
        mergeSet(finalUsedExtensions, uniqueExtensions, layersAndExtensions.avaiableExtensions );
        mergeSet(finalUsedLayers, uniqueLayers, layersAndExtensions.avaiableLayers);

        AR_LOG(Info, "** layer count:%d ** ", finalUsedLayers.size());
        for(auto&& layer : finalUsedLayers){
            AR_LOG(Info, "\t -- layer: %s", layer);
        }

        AR_LOG(Info, "** final extension count:%d ** ", finalUsedExtensions.size());
        for(auto&& extension : finalUsedExtensions){
            AR_LOG(Info, "\t -- extension: %s", extension);
        }
    }

    bool createInstance(){
        // enumerate the extension
        auto getInstanceExtensionProperties = [](const char* name, uint32& count, std::vector<VkExtensionProperties>* extensionProperties){
            return GGlobalCommands()->vkEnumerateInstanceExtensionProperties(name, &count, extensionProperties? extensionProperties->data():nullptr);
        };

        // enumerate the layer
        auto getInstanceLayerProperties = [](std::vector<VulkanOneLayerExtensions>& layerExtensions, uint32& count, std::vector<VkLayerProperties>* layers){
            VkResult ret = GGlobalCommands()->vkEnumerateInstanceLayerProperties(&count, layers ? layers->data() : nullptr);
            if(layers !=nullptr){
                AlgOps::unique(*layers, LayerPropertiesLess, LayerPropertiesEqual);
                layerExtensions.resize(layers->size()+1);
                for(uint32 i=0; i< layers->size(); ++i){
                    layerExtensions[i + 1].layerProperty = layers->data() + i;
                }
            }
            return ret;
        };
        
        std::vector<const char*> outInstanceExtension{};
        std::vector<const char*> outInstanceLayers{};
        std::vector<const char*> availableExtensions{};
        std::vector<const char*> availableLayers{};

        GetVulkanInstanceExtensionStrings(instance.layerAndExtensions.avaiableExtensions);
        instance.layerAndExtensions.avaiableLayers.clear();

        AR_LOG(Info, "*** Instance Layer and Extension: **");
        getLayerAndExtension(instance.layerAndExtensions , getInstanceLayerProperties , getInstanceExtensionProperties);

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
        ARCheck(GGlobalCommands()->vkCreateInstance!=nullptr);
        VkResult ret = GGlobalCommands()->vkCreateInstance(&CreateInfo, nullptr, &instance.handle);

        if (ret != VK_SUCCESS){
            AR_LOG(Info, "create instance raise %s error, exit!", GetVkResultString(ret));
            return false;
        }

        // get instance proc addr
        if(!instance.GetCommandAddrs()){
            AR_LOG(Info, "get instance commands  error, exit!");
            return false;
        }
        // set to global
        GInstanceCommandsRef() = &instance;
        return true;
    }
    
    bool createDevice(){
        AR_LOG(Info, "** createDevice ");
        std::vector<VkPhysicalDevice> physicalDevices;
        // enumerate physical device
        auto getPhysicalDevice = [](uint32& count, std::vector<VkPhysicalDevice>* physicalDevices){
            ARCheck(GInstanceCommands()->vkEnumeratePhysicalDevices!=nullptr);
            return GInstanceCommands()->vkEnumeratePhysicalDevices(GInstanceCommands()->handle, &count, physicalDevices? physicalDevices->data():nullptr);
        };
        enumerateProperties(physicalDevices, getPhysicalDevice);
        AR_LOG(Info, "*** physical device count : %d ***", physicalDevices.size());
        if(physicalDevices.size() == 0){
            AR_LOG(Info, " no physical found, exit!");
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
            auto getDeviceExtensionProperties = [&physicalDevice](const char* name, uint32& count, std::vector<VkExtensionProperties>* extensionProperties){
                ARCheck(GInstanceCommands()->vkEnumerateDeviceExtensionProperties!=nullptr);
                return GInstanceCommands()->vkEnumerateDeviceExtensionProperties(physicalDevice, name, &count, extensionProperties? extensionProperties->data():nullptr);
            };

            // enumerate the layer
            auto getDeviceLayerProperties = [&physicalDevice](std::vector<VulkanOneLayerExtensions>& layerExtensions, uint32& count, std::vector<VkLayerProperties>* layers){
                ARCheck(GInstanceCommands()->vkEnumerateDeviceLayerProperties!=nullptr);
                VkResult ret = GInstanceCommands()->vkEnumerateDeviceLayerProperties(physicalDevice, &count, layers ? layers->data():nullptr);
                if(layers !=nullptr){
                    AlgOps::unique(*layers, LayerPropertiesLess, LayerPropertiesEqual);
                    layerExtensions.resize(layers->size() + 1);
                    for(uint32 i=0; i< layers->size(); ++i){
                        layerExtensions[i+1].layerProperty = layers->data() + i;
                    }
                }
                return ret;
            };


            AR_LOG(Info, "*** Device Layer and Extension: **");
            getLayerAndExtension( device.layerAndExtensions,  getDeviceLayerProperties, getDeviceExtensionProperties);

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
                AR_LOG(Info, "create device raise %s error, exit!", GetVkResultString(ret));
                return false;
            }

            // get instance proc addr
            if(!device.GetCommandAddrs()){
                AR_LOG(Info, "get instance commands  error, exit!");
                return false;
            }
            // set to global
            GDeviceCommandsRef(i) = &device;

            // get queues
            if(!device.createQueue()){
                AR_LOG(Info, "create queue  error, exit!");
                return false;
            }
        }
        return true;
    }

    bool initBasicEntryPoints(){ return false; }

    bool initialize(){
        AR_LOG(Info, "vulkan context: initialize");
        Derive* derive = static_cast<Derive*>(this);
        if(! derive->initBasicEntryPoints()){
            return false;
        }
        AR_LOG(Info, "vulkan context: initBasicEntryPoints succ");

        if(!createInstance()){
            return false;
        }
        AR_LOG(Info, "vulkan context: createInstance succ");

        setupDebugLayerCallback();
        
        if(!createDevice()){
            return false;
        }
        AR_LOG(Info, "vulkan context: createDevice succ");
        return true;
    }

    bool finalize(){
        AR_LOG(Info, "vulkan context: finalize");
        if(instance.handle!=VK_NULL_HANDLE){
            GInstanceCommands()->vkDestroyInstance(instance.handle, nullptr);
            instance.handle = VK_NULL_HANDLE;
        }
        

        AR_LOG(Info, "vulkan context: destroy instance");
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

        AR_LOG(Info, "VK_EXT_debug_report:%d", (int)IsVkExtensionEnabled(VK_EXT_debug_report));
        if (IsVkExtensionEnabled(VK_EXT_debug_report) && false)
        {
            ARCheck(GInstanceCommands()->vkCreateDebugReportCallbackEXT!=nullptr);
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
                    AR_LOG(Info, " vkCreateDebugReportCallbackEXT fail:%s", GetVkResultString(ret));
                }

                AR_LOG(Info, "#enable debug report extension");
            }   
        }
    }
};

PROJECT_NAMESPACE_END