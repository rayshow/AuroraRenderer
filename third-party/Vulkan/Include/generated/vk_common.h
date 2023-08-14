#pragma once

#include<string>
#include<vector>
#include<unordered_map>

#include"type.h"
#include"platform/file_system.h"
#include"platform/common/to_string_protocol.h"
#include"platform/common/raw_string.h"
#include"hash.h"

#if RS_PLATFORM_DEFINE == RS_PLATFORM_ANDROID
#define VK_USE_PLATFORM_ANDROID_KHR 1
#elif RS_PLATFORM_DEFINE == RS_PLATFORM_WINDOW
#define VK_USE_PLATFORM_WIN32_KHR 1
#else 
#error "not implements"
#endif

#include "vulkan/vulkan.h"
#if BUILD_VULKAN_LAYER
#include "vulkan/vk_layer.h"
#endif
#include"vk_dispatch_defs.h"
#include"vk_enum_defs.h"

using EmitFatalFn = void(*)(VkResult);
inline EmitFatalFn emitFatalCall = nullptr;
inline VkResult checkVulkanResultWithLoc(VkResult code, bool emitFatal, char const* expr, char const* file, int line){
    if(code!=VK_SUCCESS){
        RS_LOG("invalid VkResult:%s at Expr:%s file:%s line:%d", GetVkResultString(code), expr, file, line );
        RS_LOG_FLUSH();
        if(emitFatal && (code == VK_ERROR_DEVICE_LOST || code == VK_ERROR_INITIALIZATION_FAILED)){
            //emitFatalCall(code);
            if(emitFatalCall) emitFatalCall(code);
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

#define LOG_ERROR_IF_API_NOT_EXISTS 0

#if LOG_ERROR_IF_API_NOT_EXISTS
#define CheckAPIExists(Api) if(Api==nullptr){ RS_LOG("%s not exists", #Api); return; }
#else 
#define CheckAPIExists(Api) if(Api==nullptr){ return; }
#endif

PROJECT_NAMESPACE_BEGIN

struct ObjectInfo{
    using ThisType = ObjectInfo;
    using SuperType = void;

    ObjectInfo()=default;
    virtual ~ObjectInfo(){};

    virtual uint32 GetClassUniqueID() const{
        return 0;
    }

    virtual char const* GetClassName() const{
        static char const ClassName[]={"ObjectInfo"};
        return ClassName;
    }

    // dispatch table
    using CreateObjectFn = ObjectInfo* (void);
    inline static std::unordered_map<uint32, CreateObjectFn*> SCreateMap{};
    static inline void RegisterCreate(uint32 uid, CreateObjectFn* fn ){
        rs_check(SCreateMap.find(uid) == SCreateMap.end());
        SCreateMap.emplace(uid, std::move(fn));
    }
    static CreateObjectFn* GetCreate(uint32 uid){
        auto&& it = SCreateMap.find(uid);
        if( it!=SCreateMap.end()){
            return (it->second);
        }
        return nullptr;
    }
    
    virtual bool serialize(File* file) const{ return true;};
    virtual bool deserialize(File* file){ return true;};
    virtual std::string toString() const{
        static std::string empty{""};
        return empty;
    }

    // called after deserialize
    virtual void setupAfterDeserialize(){
        RS_LOG("ObjectInfo::setupAfterDeserialize call");
    }

    // write uid to distinguish object
    bool polymorphismSerialize(File* file) const {
        bool succ = true;
        CHECK_SERIALIZE_SUCC(file->write(GetClassUniqueID()));
        return serialize(file);
    }

     // create derive object
    template<typename T>
    static T* polymorphismCreate(uint32 uid){
        CreateObjectFn* createFn = GetCreate(uid);
        if(createFn == nullptr){
            return nullptr;
        }
        ObjectInfo* pBase = createFn();
        return static_cast<T*>(pBase);
    }

    // create derive object
    template<typename T>
    static T* polymorphismDeserialize(File* file){
        uint32 uid = 0;
        file->read(uid);
        T* pBase= polymorphismCreate<T>(uid);
        if(!pBase){
            return nullptr;
        }
        if(!pBase->deserialize(file)){
            delete pBase;
            pBase=nullptr;
        }
        return pBase;
    }
};

template<typename T>
struct RegisterObjectCreateFn{
    RegisterObjectCreateFn(){ ObjectInfo::RegisterCreate(T::SUID, &T::CreateThisObject ); }
};

template<typename T, typename R = typename T::SuperType> struct root_class: public root_class<R>{};
template<typename T> struct root_class<T,void>{ using type = T; };
template<typename T> using root_class_t = typename root_class<T>::type;


#define RS_DECLARE_OBJECT_INFO_CLASS(ThisClass, SuperClass)                                              \
            public: using ThisType = ThisClass;                                                          \
            using SuperType = SuperClass;                                                                \
            static constexpr char kClassUniqueName[] = { RS_STRINGIZE(SuperClass ## _ ## ThisClass) };   \
            static constexpr char kClassBaseClassName[] = { #SuperClass };                               \
            static constexpr char kClassThisClassName[] = { #ThisClass };                                \
            inline static UID32 SUID{kClassUniqueName};                                                  \
            virtual char const* GetClassName() const override{ return kClassThisClassName; }             \
            virtual uint32 GetClassUniqueID() const override {  return SUID; }                           \
            static root_class_t<ThisType>* CreateThisObject(){ return new ThisClass(); }                 \
            inline static RegisterObjectCreateFn<ThisType> RegisterInfo{};
 
#define RS_BEGIN_SERIALIZE \
         bool succ = true; \
         FILE_SYSTEM_DEBUG_LOG("serialize: %s", kClassThisClassName);

#define RS_BEGIN_BASE_SERIALIZE  \
            bool succ = true;    \
            FILE_SYSTEM_DEBUG_LOG("serialize: %s", kClassThisClassName); \
            CHECK_SERIALIZE_SUCC(SuperType::serialize(file));

#define RS_BEGIN_BASE_DESERIALIZE \
            bool succ = true;     \
            FILE_SYSTEM_DEBUG_LOG("deserialize: %s", kClassThisClassName); \
            CHECK_SERIALIZE_SUCC(SuperType::deserialize(file));

#define RS_END_SERIALIZE  return succ;


enum class EHandleOperateType{
    create,
    destroy,
    replace,
    get, // may no destroy, replace old
};

enum class EVkCommandType{
    create,
    destroy,
    other
};

// use in execute callback
inline uint64_t GUnusedHandle = 0;
using HandleReplace = void(uint64_t oldHandle,uint64_t& newHandle, EHandleOperateType type, char const* variableName, int line);


enum class ECommandConstructType{
    // just reintercept_cast then VkCommand call, don't free
    fromLocal,
    // from read, read must allocate memory to hold the memory, and also need process free
    fromAllocate
};


struct DeepCopy{};
constexpr DeepCopy kDeepCopy{};

struct ReplayDispatch{
    template<typename... Args>
    static  void dispatch(Args&&... args){
        RS_LOG("execute cmd");
    }
     template<typename... Args>
    static void dispatch(char const* name, struct SCreateDevice& createDeviceCmd, Args&&... args){
        RS_LOG("execute createDevice");
    }
};

// vulkan command
struct CommandInfo: public ObjectInfo
{
    RS_DECLARE_OBJECT_INFO_CLASS(CommandInfo,ObjectInfo)
    ECommandConstructType constructType = ECommandConstructType::fromLocal;
    inline static ReplayDispatch* dispatcher = nullptr;

    CommandInfo()=default;
    CommandInfo(DeepCopy){ constructType = ECommandConstructType::fromAllocate;  }
 
    virtual bool deserialize(File* file)override {
        // pointer from read all dynamic allocate, should free mannual
        constructType = ECommandConstructType::fromAllocate;
        return true;
    }

    void conditionalDestroy(){
        if(constructType == ECommandConstructType::fromAllocate){
            deepDestroy();
        }
    }

    // recycle all resource allocated
    virtual void deepDestroy(){}

    // all command can execute
    virtual void execute(HandleReplace&& handleReplace){}

    virtual std::string toString() const override {
        RS_BEGIN_TO_STRING(CommandInfo);
        RS_TO_STRING(constructType);
        RS_END_TO_STRING();
    }
    

    template<typename... Args>
    void callDispatch(char const* apiName, Args&&... args){
        if(dispatcher){
            dispatcher->dispatch(std::forward<Args>(args)...);
        }
    }
};


template<typename T>
RS_FORCEINLINE T* typedPointer(void const* data){
    return const_cast<T*>( reinterpret_cast<const T*>(data));
}

template<typename T, typename R>
RS_FORCEINLINE T* castDerived(R const* base ){
    return  const_cast<T*>( static_cast<T const*>(base));
}


/**
 * @brief  a c vulkan struct zero-overhead virtual table
 * 
 * @tparam T 
 */
struct VkLocalStruct{
    using LocalConstructor = void*();
    using LocalDestructor = void(void const*);
    using LocalDeepCopy = void(void* dest, const void* src);
    using LocalSerialize = bool(File*, const void* data);
    using LocalDeserialize = bool(File*, const void* data);
    using ClassSize        = uint32();

    LocalConstructor*   defaultConstructor;
    LocalDestructor*    destructor;
    LocalDeepCopy*      objectDeepCopy;
    LocalSerialize*     serialize;
    LocalDeserialize*   deserialize;
    ClassSize*          classSize;

    inline static std::unordered_map<VkStructureType, VkLocalStruct*> SMappings{};

    static void registerCallback(VkStructureType type, VkLocalStruct* local){
        rs_check(SMappings.find(type) == SMappings.end());
        SMappings.emplace(type, local);
    }

    static VkLocalStruct* getLocalFn(VkStructureType type){
        auto found = SMappings.find(type);
        rs_checkf( found!= SMappings.end(), "struct type:%s %d", GetVkStructureTypeString(type),(int)type);
        return found->second;
    }


    static bool serializeNext(File* file, void const* pNext){
        bool succ=true; 
        if(pNext){
            FILE_SYSTEM_DEBUG_LOG("serialize next exists: %p", pNext);
            VkBaseInStructure* base = typedPointer<VkBaseInStructure>(pNext);
            CHECK_SERIALIZE_SUCC(file->write(base->sType));
            CHECK_SERIALIZE_SUCC(getLocalFn(base->sType)->serialize(file, pNext));
        }else{
            FILE_SYSTEM_DEBUG_LOG("serialize next not exists");
            CHECK_SERIALIZE_SUCC(file->write(VK_STRUCTURE_TYPE_MAX_ENUM));
        }
        return succ;
    }

    template<typename T>
    static bool deserializeNext(File* file, T** pNext){
        bool succ=true; 
        VkStructureType type;
        CHECK_SERIALIZE_SUCC(file->read(type));
        if(type!=VK_STRUCTURE_TYPE_MAX_ENUM){
            VkLocalStruct* localStruct = getLocalFn(type);
            void* pNextData = localStruct->defaultConstructor();
            rs_check(pNextData!=nullptr);
            CHECK_SERIALIZE_SUCC(localStruct->deserialize(file, pNextData));
            *pNext = pNextData;
        }
        return succ;
    }

    static VkBaseOutStructure* deepCopy(void const* pNext){
        bool succ=true; 
        if(pNext){
            VkBaseInStructure* base = typedPointer<VkBaseInStructure>(pNext);
            VkLocalStruct* localStruct = getLocalFn(base->sType);
            // create a copy first
            VkBaseOutStructure* newCopy = typedPointer<VkBaseOutStructure>( localStruct->defaultConstructor() );
            rs_check(newCopy!=nullptr);
            // pod copy
            memcpy(newCopy, base, localStruct->classSize());
            localStruct->objectDeepCopy(newCopy, base );
            // recursive copy next
            newCopy->pNext = typedPointer<VkBaseOutStructure>( VkLocalStruct::deepCopy(base->pNext));
            return newCopy;
        }
        return nullptr;
    }

    static void deepDestroy(void const* pNext){
        if(pNext){
            VkBaseInStructure* base = typedPointer<VkBaseInStructure>(pNext);
            VkLocalStruct* localStruct = getLocalFn(base->sType);
            // recursive deep destroy
            VkLocalStruct::deepDestroy(base->pNext);
            // then current class 
            localStruct->destructor(pNext);
        }
    }

    template<typename T>
    struct LocalRegister{
        LocalRegister(){
            registerCallback((VkStructureType)T::kStructType, &T::Functions);
        }
    };
};

// use structType for vulkan struct has structType
#define RS_DECLARE_VK_TYPE_STRUCT(ThisClass, Struct, StructEnum)                                            \
            public: using ThisType = ThisClass;                                                             \
            static constexpr VkStructureType kStructType = StructEnum;                                      \
             static constexpr char kClassThisClassName[] = { #ThisClass };                                  \
            static void* LocalCreate(){ return new ThisClass(); }                                           \
            static void  LocalDestroy(void const* data){ delete typedPointer<ThisClass>(data); }            \
            static bool LocalSerialize(File* file, void const* data){                                       \
                return typedPointer<ThisClass>(data)->serialize(file); }                                    \
            static bool LocalDeserialize(File* file, void const* data){                                     \
                return typedPointer<ThisClass>(data)->deserialize(file); }                                  \
            static void LocalDeepCopy(void* dest, void const* src){                                        \
                      typedPointer<ThisClass>(dest)->deepCopy(*typedPointer<Struct>(src)); }               \
            static uint32 ClassSize(){ return sizeof(ThisType); }                                           \
            inline static VkLocalStruct Functions{                                                          \
                ThisClass::LocalCreate, ThisClass::LocalDestroy, ThisClass::LocalDeepCopy,                  \
                ThisClass::LocalSerialize, ThisClass::LocalDeserialize, ThisClass::ClassSize };             \
            inline static VkLocalStruct::template LocalRegister<ThisClass> RegisterInfo{};                  

#define RS_DECLARE_VK_POD_STRUCT(ThisClass)  \
            public: using ThisType = ThisClass;                                                          \
            static constexpr char kClassThisClassName[] = { #ThisClass };                               


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
    rs_check(GInstanceCommandsRef() !=nullptr);
    return GInstanceCommandsRef();
}

// VkDevice handle and batch Array with VkCommand associate with it
constexpr int kMaxPhysicalDevices=4;
inline VkDevDispatchTable*& GDeviceCommandsRef(int index=0){
    static VkDevDispatchTable* dispatchTables[kMaxPhysicalDevices]{nullptr};
    return dispatchTables[index];
}

inline VkDevDispatchTable* GDeviceCommands(int index=0){
    rs_check(GDeviceCommandsRef(index)!=nullptr);
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

#define VkHandleProcess(oldHandle, newHandle, Type)  handleReplace((uint64_t)oldHandle, newHandle, Type, #oldHandle, __LINE__)

inline std::string GetDeviceHeapFlagString(VkMemoryPropertyFlags flag){
    std::string ret; 
    if( (flag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT){
        ret+=" DEVICE_LOCAL ";
    }
    if((flag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT){
        ret+=" HOST_VISIBLE ";
    }
    if((flag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){
        ret+=" HOST_COHERENT ";
    }
    if((flag & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT){
        ret+=" HOST_CACHED ";
    }
    return ret;
}

inline void DumpPhysicalMemoryProp( VkPhysicalDeviceMemoryProperties& memoryProperties){
    RS_LOG("\n** memory heap info **");
    RS_LOG("** type count:%d, heap cout:%d", memoryProperties.memoryTypeCount, memoryProperties.memoryHeapCount);

    for(uint32 i=0; i< memoryProperties.memoryTypeCount; ++i){
        VkMemoryType& type = memoryProperties.memoryTypes[i];
        RS_LOG("-- memory type[%d], heap index:%d, property:%s", i, type.heapIndex, GetDeviceHeapFlagString(type.propertyFlags).c_str());
    }
    for(uint32 i=0; i<memoryProperties.memoryHeapCount; ++i ){
        VkMemoryHeap& heap = memoryProperties.memoryHeaps[i]; 
        RS_LOG("-- heap type[%d], heap size:%lld(%f MB), flagsd:%d", i, heap.size, (double)heap.size / (1024.0*1024.0), heap.flags);
    }
}
 
 
struct ObjectMemoryInfo{
    // buffer, image
    uint32_t objectHandle;
    VkDeviceSize offset;
    VkDeviceSize size;
    // memory
    
    VkDeviceMemory memory;
    VkDeviceSize allocationSize;
    VkMemoryPropertyFlags flag;

    std::string toString() const{
        RS_BEGIN_TO_STRING(ObjectMemoryInfo);
        RS_TO_STRING(offset);
        RS_TO_STRING(size);
        RS_TO_STRING((void*)memory);
        RS_TO_STRING(allocationSize);
        RS_TO_STRING_WITH_NAME(flag, GetDeviceHeapFlagString(flag))
        RS_END_TO_STRING()
    }
    bool isHostVisible() const{
        return (flag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    bool isHostCoherent() const{
        return (flag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
};

struct SHostCoherentMemoryCopy:public CommandInfo{
	static constexpr EVkCommandType kCommandType=EVkCommandType::other;
	RS_DECLARE_OBJECT_INFO_CLASS(SHostCoherentMemoryCopy, CommandInfo)
	
    VkCommandBuffer cmdBuffer;
	VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize range;
	uint8_t* data; 

	SHostCoherentMemoryCopy()=default;
	SHostCoherentMemoryCopy( VkCommandBuffer inCmdBuffer, VkDeviceMemory inMemory, VkDeviceSize inOffset, VkDeviceSize inRange, uint8_t const* inData,DeepCopy)
		: CommandInfo{kDeepCopy}
		, cmdBuffer{inCmdBuffer}
        , memory{inMemory}
        , offset{inOffset}
        , range{inRange}
        , data{ptr::safe_new_copy_array(inData, inRange)}
	{
        rs_check(data!=nullptr);
	}
 
	virtual ~SHostCoherentMemoryCopy() override{conditionalDestroy();};
	virtual std::string toString() const override{
		RS_BEGIN_DERIVED_TO_STRING(SHostCoherentMemoryCopy,CommandInfo);
		RS_TO_STRING(cmdBuffer);
		RS_TO_STRING(memory);
		RS_TO_STRING(offset);
		RS_TO_STRING(range);
		RS_TO_STRING(data);
		RS_END_TO_STRING();
	}
	virtual bool serialize(File* file) const override{
		RS_BEGIN_BASE_SERIALIZE
		CHECK_SERIALIZE_SUCC(file->write(cmdBuffer));
		CHECK_SERIALIZE_SUCC(file->write(memory));
		CHECK_SERIALIZE_SUCC(file->write(offset));
		CHECK_SERIALIZE_SUCC(file->write(data, range, kReadWriteLength_Allocate));
		RS_END_SERIALIZE
	}
	virtual bool deserialize(File* file) override{
		RS_BEGIN_BASE_DESERIALIZE
		CHECK_SERIALIZE_SUCC(file->read(cmdBuffer));
		CHECK_SERIALIZE_SUCC(file->read(memory));
		CHECK_SERIALIZE_SUCC(file->read(offset));
		CHECK_SERIALIZE_SUCC(file->read(data, range, kReadWriteLength_Allocate));
		RS_END_SERIALIZE
	}
	virtual void execute(HandleReplace&& handleReplace) override {

	}
	virtual void deepDestroy() override{
		ptr::safe_delete_array(data, range);
	}
};


enum EScopeState:uint8{
    Unset =0,
    Begin,
    End,
};

enum class EMemoryUsage{
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    Indirect,
    Image
};

struct VkMemoryRange{
    VkDeviceMemory     memory;
    uint64             handle;
    VkDeviceSize       offset;
    VkDeviceSize       length;
};

struct SMapMemory;


struct BindIndexBuffer{
    VkBuffer  buffer;
    VkDeviceMemory  memory;
    VkDeviceSize offset;
    VkDeviceSize length;
    VkIndexType  indexType;
    bool outdated;

    BindIndexBuffer()
        : buffer{VK_NULL_HANDLE}
        , memory{VK_NULL_HANDLE}
        , offset{0}
        , length{0}
        , indexType{VK_INDEX_TYPE_MAX_ENUM}
        , outdated{true}
    {}

    uint32 getByteStride() const{
        switch(indexType){
            case VK_INDEX_TYPE_UINT16: return 2;
            case VK_INDEX_TYPE_UINT32: return 4;
            case VK_INDEX_TYPE_UINT8_EXT: return 1;
            default: break;
        }
        rs_check(false);
        return 0;
    }

    std::string toString() const{
		RS_BEGIN_TO_STRING(BindIndexBuffer);
		RS_TO_STRING( buffer );
		RS_TO_STRING( memory );
        RS_TO_STRING( offset );
        RS_TO_STRING( length );
        RS_TO_STRING_WITH_NAME( indexType,  getByteStride() );
        RS_TO_STRING( outdated );
		RS_END_TO_STRING();
	}

};

struct BindVertexBuffer{
    VkBuffer  buffer;
    VkDeviceMemory  memory;
    VkDeviceSize offset;
    VkDeviceSize length;
    VkVertexInputBindingDescription binding;
    int16 zeroStrideSize;
    bool outdated;

    BindVertexBuffer()
        : buffer{VK_NULL_HANDLE}
        , memory{VK_NULL_HANDLE}
        , offset{0}
        , length{0}
        , binding{}
        , zeroStrideSize{0}
        , outdated{true}
    {}

    std::string toString() const{
		RS_BEGIN_TO_STRING(BindVertexBuffer);
        RS_TO_STRING_WITH_NAME( index,  binding.binding );
        RS_TO_STRING_WITH_NAME( rate,  (int)binding.inputRate );
        RS_TO_STRING_WITH_NAME( stride,  (int)binding.stride );
		RS_TO_STRING( buffer );
		RS_TO_STRING( memory );
        RS_TO_STRING( offset );
        RS_TO_STRING( length );
        RS_TO_STRING( outdated );
		RS_END_TO_STRING();
	}

};

struct CommandBufferScope{
    // which buffer
    
    EScopeState state = EScopeState::Unset;

    using MemoryRanges = std::vector< VkMemoryRange>;
    // used image/buffer's memory mapped
    MemoryRanges uniformRanges;
    MemoryRanges indexBufferRanges;
    MemoryRanges vertexBufferRanges;
    MemoryRanges imageRanges;
    MemoryRanges indirectRanges;


    VkPrimitiveTopology topology;
    BindIndexBuffer indexBuffer;
    BindVertexBuffer vertexBuffers[16];


    void begin(){
        state = EScopeState::Begin;
        uniformRanges.clear();
        indexBufferRanges.clear();
        vertexBufferRanges.clear();
        imageRanges.clear();
        indirectRanges.clear();
    }
    void end(){
        state = EScopeState::End;
    }

    bool isBegined() const{
        return state == EScopeState::Begin;
    }
    bool isEnded() const{
        return state == EScopeState::End;
    }

    void bindIndexBuffer( VkBuffer  buffer, VkDeviceMemory memory, VkDeviceSize  offset, VkDeviceSize length, VkIndexType  indexType){
        indexBuffer.buffer = buffer;
        indexBuffer.memory = memory;
        indexBuffer.offset = offset;
        indexBuffer.length = length;
        indexBuffer.indexType = indexType;
        indexBuffer.outdated= false;
    }

    void bindVertexBuffer( uint32 index, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize  offset, VkDeviceSize length){
        rs_check(index < 16);
        vertexBuffers[index].buffer = buffer;
        vertexBuffers[index].memory = memory;
        vertexBuffers[index].offset = offset;
        vertexBuffers[index].length = length;
        vertexBuffers[index].outdated = false;
    }

    void setVertexBinding(uint32 index, VkVertexInputBindingDescription const& binding){
        rs_check(index < 16);
        vertexBuffers[index].binding = binding;
    }

    void setVertexBindingZeroStrideSize(uint32 index, uint32 size){
        rs_check(size>0);
        vertexBuffers[index].zeroStrideSize = size;
    }

    template<typename Fn>
    void foreachVertexBuffer(Fn&& fn){
        for(uint32 i=0; i<16; ++i){
            if(!vertexBuffers[i].outdated){
                rs_check(vertexBuffers[i].buffer!=VK_NULL_HANDLE);
                rs_check(vertexBuffers[i].length>0);
                rs_check(vertexBuffers[i].binding.stride>=0);
                fn(vertexBuffers[i]);
            }
        }
    }

    std::pair<uint32, uint32> getVertexCountByIndex(uint32 offset, uint32 count){
        std::pair<uint32, uint32> result;
        result.first = 0;

        rs_check(count > 0);
        switch (topology)
        {
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            rs_check(count % 3 == 0);
            result.second = count;
            break;
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
            result.second = count;
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            rs_check(count % 2 == 0 );
            result.second = count / 2 +1;
        default:
            rs_check(false);
            break;
        }
        return result;
    }

    bool addMemoryCopy(VkDeviceMemory memory, uint64 buffer, EMemoryUsage usage,  VkDeviceSize offset, VkDeviceSize length){
        MemoryRanges* rangeArray = nullptr;
        switch (usage)
        {
        case EMemoryUsage::Image:
            rangeArray = &imageRanges;
            break;
        case EMemoryUsage::IndexBuffer:
            rangeArray = &indexBufferRanges;
            break;
        case EMemoryUsage::UniformBuffer:
            rangeArray = & uniformRanges;
            break;
        case EMemoryUsage::Indirect:
            rangeArray = &indirectRanges;
            break;
        default:
            break;
        }
        rs_check(rangeArray!=nullptr);
        for(auto&& range : *rangeArray) {
            // exists
            if(range.memory == memory 
                && range.handle == buffer 
                && range.offset == offset 
                && range.length == length ) 
            {
                return false;
            }
        }
        VkMemoryRange range;
        range.length = length;
        range.handle = buffer;
        range.memory = memory;
        range.offset = offset;
        rangeArray->push_back(range);
        return true;
    }

    bool addVertexMemoryCopy(VkDeviceMemory memory, uint64 buffer,  VkDeviceSize offset, VkDeviceSize length){
        bool foundRange = false;
        uint32 i =0;
        for(; i<vertexBufferRanges.size(); ++i){
            if(vertexBufferRanges[i].memory == memory 
                && vertexBufferRanges[i].handle == buffer
                && vertexBufferRanges[i].offset == offset)
            {
                foundRange = true;
                break;
            }
        }
        if(foundRange){
            //ranges[i].length = std::max(ranges[i].length, length);
            if(length > vertexBufferRanges[i].length){
                vertexBufferRanges[i].length = length;
            }
            return false;
        }
        VkMemoryRange range;
        range.memory = memory;
        range.offset = offset;
        range.length = length;
        range.handle = buffer; 
        vertexBufferRanges.emplace_back(range);
        return true;
    }

    template<typename Fn>
    void foreachRange(Fn&& fn){
        for(auto&& range:vertexBufferRanges){ fn(range, EMemoryUsage::VertexBuffer, "VertexBuffer"); }
        for(auto&& range:indexBufferRanges){ fn(range, EMemoryUsage::IndexBuffer, "IndexBuffer"); }
        for(auto&& range:uniformRanges){ fn(range, EMemoryUsage::UniformBuffer, "UniformBuffer"); }
        for(auto&& range:imageRanges){ fn(range, EMemoryUsage::Image, "Image"); }
        for(auto&& range:indirectRanges){ fn(range, EMemoryUsage::Indirect, "Indirect"); }
    }
 
    std::string toString() const{
		RS_BEGIN_TO_STRING(CommandBufferScope);
		RS_TO_STRING( state );
		RS_TO_STRING( vertexBufferRanges );
        RS_TO_STRING( indexBufferRanges );
        RS_TO_STRING( uniformRanges );
        RS_TO_STRING( imageRanges );
        RS_TO_STRING( indirectRanges );
		RS_END_TO_STRING();
	}
};


inline void LogFloatArray(char const* prefix, void const* data, uint32 length){
    if(data && length > 0){
        float const* fdata = reinterpret_cast<float const*>(data);
        RS_LOG("%s: %d  %s",prefix, length, ToStringProtocol::toString(fdata, length/4).c_str());
    }
}

inline void LogUint32Array(char const* prefix, void const* data, uint32 length){
    if(data && length > 0){
        uint32 const* idata = reinterpret_cast<uint32 const*>(data);
        RS_LOG("%s: %s",prefix,  ToStringProtocol::toString(idata, length/4).c_str());
    }
}

inline void LogUint16Array(char const* prefix, void const* data, uint32 length){
    if(data && length > 0){
        uint16 const* idata = reinterpret_cast<uint16 const*>(data);
        RS_LOG("%s: %s",prefix,  ToStringProtocol::toString(idata, length/2).c_str());
    }
}

enum class EDescriptorTypePostion{
    Undefined,
    ImageSamplerInfo,
    ImageViewInfo,
    ImageCombinedInfo,
    BufferInfo,
    DynamicBufferInfo,
    TexelBufferView,
    Inline,
};

inline EDescriptorTypePostion GetDescriptorTypePosition(VkDescriptorType type){
    switch (type)
    {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
        //pimageInfo. sampler
        return EDescriptorTypePostion::ImageSamplerInfo;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        //pimageInfo. imageView / imageLayout
        return EDescriptorTypePostion::ImageViewInfo;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        //pimageInfo. imageView / imageLayout / sampler
        return EDescriptorTypePostion::ImageCombinedInfo;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        //pBufferinfo
        return EDescriptorTypePostion::BufferInfo;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return EDescriptorTypePostion::DynamicBufferInfo;
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER :
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        //pTexelBufferView 
        return EDescriptorTypePostion::TexelBufferView;
    case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
        return EDescriptorTypePostion::Inline;
    default:
        return EDescriptorTypePostion::Undefined;
    }
    return EDescriptorTypePostion::Undefined;
}


PROJECT_NAMESPACE_END
