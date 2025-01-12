#pragma once

#include"core/util/singleton.h"
#include"core/util/execute_if_fail.h"
#include"HAL/app_config.h"
#include"dx12_header.h"
#include"dx12_device.h"
#include"assert.h"
#include"core/util/util.h"

PROJECT_NAMESPACE_BEGIN
#define MAX_GPUS 2

class DX12Context : public Singleton< DX12Context>
{
private:
	TRefCountPtr<IDXGIFactory2> _dxgiFactory2;
	TRefCountPtr<IDXGIFactory4> _dxgiFactory4;
	TRefCountPtr<IDXGIFactory5> _dxgiFactory5;
	TRefCountPtr<IDXGIFactory6> _dxgiFactory6;
	TRefCountPtr<IDXGIFactory7> _dxgiFactory7;

	// all adapter
	TArray<TRefCountPtr<IDXGIAdapter1>> _adapters{};

	// current adapter
	DXGI_ADAPTER_DESC1  _adapterDesc{};
	TRefCountPtr<IDXGIAdapter1> _adapter{};
	D3D_FEATURE_LEVEL           _featureLevel{D3D_FEATURE_LEVEL_1_0_CORE};
	TRefCountPtr<ID3D12Device>  _device{};
	TRefCountPtr<ID3D12Device1> _device1{};
	TRefCountPtr<ID3D12Device2> _device2{};
	TRefCountPtr<ID3D12Device3> _device3{};
	TRefCountPtr<ID3D12Device4> _device4{};
	TRefCountPtr<ID3D12Device5> _device5{};
	TRefCountPtr<ID3D12Device6> _device6{};
	TRefCountPtr<ID3D12Device7> _device7{};
	TRefCountPtr<ID3D12Device8> _device8{};
	TRefCountPtr<ID3D12Device9> _device9{};
	TRefCountPtr<ID3D12Device10> _device10{};

	i32                                      _nodeCount{ 0 };
	TStaticArray< DX12DeviceNode*, MAX_GPUS> _deviceNodes{};
	
	static constexpr D3D_FEATURE_LEVEL GWantsFeatureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};

	
	void printAdapterInfo(i32 index, TRefCountPtr<IDXGIAdapter1>& Adapter) {
		DXGI_ADAPTER_DESC1 Desc;
		DX12_CHECK_SUCC(Adapter->GetDesc1(&Desc));

		auto description = RawStrConvert<wchar, char>::to(Desc.Description);

		AR_LOG(Info, "** Adapter%d ", index);
		AR_LOG(Info, "*  Vendor code:0x%x name:%s Desc:%s", Desc.VendorId,
			D3DGetGPUVenderString(EGPUVender(Desc.VendorId)), description.get() );
		AR_LOG(Info, "*  Luid %d-%d", Desc.AdapterLuid.HighPart, Desc.AdapterLuid.LowPart);
		AR_LOG(Info, "*  SubSystem Id:%d Revision:%d DeviceId:%d Flags:%d",
			Desc.SubSysId, Desc.Revision, Desc.DeviceId, Desc.Flags);
		AR_LOG(Info, "*  Dedicated Memory: %lfMB Video, %lfMB System, Shared System Memory:%lfMB",
			AlgOps::MB(Desc.DedicatedVideoMemory),
			AlgOps::MB(Desc.DedicatedSystemMemory),
			AlgOps::MB(Desc.SharedSystemMemory));
		AR_LOG(Info, "**");
	}

	/*
	*	EnumAdapterByGpuPreference:
		DXGI_GPU_PREFERENCE_UNSPECIFIED is equivalent to calling IDXGIFactory1::EnumAdapters1
		DXGI_GPU_PREFERENCE_MINIMUM_POWER :
			1. iGPUs (integrated GPUs)
			2. dGPUs (discrete GPUs)
			3. xGPUs (external GPUs)
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE:
			1. xGPUs
			2. dGPUs
			3. iGPUs
	*/
	bool enumAdapter(DXGI_GPU_PREFERENCE gpuPreference, TRefCountPtr<IDXGIAdapter1>& Adapter) {
		i32 index = 0;
		TRefCountPtr<IDXGIAdapter1> LocalAdapter;
		if (_dxgiFactory6.isValid()) {
			while (DXGI_ERROR_NOT_FOUND !=_dxgiFactory6->EnumAdapterByGpuPreference(index, gpuPreference, DX12_GET_REF_PTR(LocalAdapter) )) {
				_adapters.push_back(LocalAdapter);
				++index;
			}
		}
		else if(_dxgiFactory2.isValid()) {
			while (DXGI_ERROR_NOT_FOUND != _dxgiFactory2->EnumAdapters1(0, Adapter.getInitAddress())) {
				_adapters.push_back(LocalAdapter);
				++index;
			}
		}
		return _adapters.size() > 0;
	}

	bool getHighestFeatureLevelDevice( 
		TArrayView< D3D_FEATURE_LEVEL> const& availableFeatureLevels,
		DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE )
	{
		// enum all adapter
		{
			TRefCountPtr<IDXGIAdapter1> Adapter;
			if (enumAdapter(gpuPreference, Adapter)) {
				AR_LOG(Info, "*** %d Adapters: ***", _adapters.size());
				for (i32 i = 0; i < _adapters.size(); ++i) {
					ARCheck(_adapters[i].isValid());
					printAdapterInfo(i, _adapters[i]);
				}
				AR_LOG(Info, "*** \n", _adapters.size());
			}
		}
		
		// find first highest feature level device
		TRefCountPtr<ID3D12Device> Device;
		for (i32 i = 0; i < availableFeatureLevels.length(); ++i) {
			for (i32 j = 0; j < _adapters.size(); ++j) {
				if (SUCCEEDED(D3D12CreateDevice(_adapters[j].getReference(), availableFeatureLevels[i], DX12_GET_REF_PTR(Device)))) {
					ARCheck(Device.isValid());
					_adapter = _adapters[j];
					_device = Device;
					_featureLevel = availableFeatureLevels[i];
					_device->SetName(L"Device");
					AR_LOG(Info, "*** Select Adapter%d Device with FeatureLevel:%s", j,
						D3DGetFeatureLevelString(_featureLevel) );
					return true;
				}
			}
		}
		return false;
	}

public:

	AR_FORCEINLINE i32 nodeMask(i32 index)
	{
		return nodeCount() == 1 ? 0 : (1 << (1 + index));
	}

	AR_FORCEINLINE i32 nodeCount() const {
		return _device->GetNodeCount();
	}

	DX12DeviceNode* getDevice(u32 index)
	{
		ARCheck(index < _deviceNodes.size());
		return _deviceNodes[index];
	}
	
	TRefCountPtr<ID3D12RootSignature> createRootSignature( u32  nodeMask, const void *pBlobWithRootSignature, u64  blobLengthInBytes )
	{
		TRefCountPtr<ID3D12RootSignature> rootSignature{};
		if( !DX12_ENSURE_SUCC( _device->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, IID_PPV_ARGS(rootSignature.getInitAddress())))){
			AR_LOG(Error, "*** Create RootSignature failed with nodeMask %d pBlobWithRootSignature:%p bytes:%lld", nodeMask, pBlobWithRootSignature, blobLengthInBytes );
		}
		return rootSignature;
	}

	TRefCountPtr<ID3D12RootSignature> createRootSignature(u32 nodeMask, D3D12_ROOT_SIGNATURE_DESC const &desc)
	{
		TRefCountPtr<ID3DBlob> signature;
		TRefCountPtr<ID3DBlob> error;
		if(!DX12_ENSURE_SUCC(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, signature.getInitAddress(), error.getInitAddress()))){
			AR_LOG(Error, "*** D3D12SerializeRootSignature create failed");
			return nullptr;
		}
		return createRootSignature(nodeMask, signature->GetBufferPointer(), signature->GetBufferSize() );
	}

	TRefCountPtr<ID3D12PipelineState> createGraphicsPipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC const& Desc)
	{
		TRefCountPtr<ID3D12PipelineState> pipelineState{};
		if( !DX12_ENSURE_SUCC(_device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(pipelineState.getInitAddress())))){
			AR_LOG(Error, "*** Create GraphicsPipeline failed");
		}
		return pipelineState;
	}

	TRefCountPtr<ID3D12DescriptorHeap> createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC const& Desc)
	{
		TRefCountPtr<ID3D12DescriptorHeap> heap{};
		if( !DX12_ENSURE_SUCC(_device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(heap.getInitAddress()) ))){
			AR_LOG(Error, "*** Create Descriptor Heap failed");
		}
		return heap;
	}

	u64 getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
	{
		return _device->GetDescriptorHandleIncrementSize(type);
	}

	void createRenderTargetView(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle )
	{
		_device->CreateRenderTargetView(resource, desc, handle);
	}

	TRefCountPtr<ID3D12CommandAllocator> createCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
	{
		TRefCountPtr<ID3D12CommandAllocator> commandAllocator{};
		if( !DX12_ENSURE_SUCC(_device->CreateCommandAllocator(type, IID_PPV_ARGS(commandAllocator.getInitAddress())))){
			AR_LOG(Error, "*** failed Create Command allocator failed");
		}
		return commandAllocator;
	}

	TRefCountPtr<ID3D12GraphicsCommandList> createCommandList(u32 nodeMask, D3D12_COMMAND_LIST_TYPE type,
		TRefCountPtr<ID3D12CommandAllocator>const & allocator, TRefCountPtr<ID3D12PipelineState> const& pipelineState)
	{
		TRefCountPtr<ID3D12GraphicsCommandList> Commandlist{};
		if( !DX12_ENSURE_SUCC(_device->CreateCommandList(nodeMask,  type, allocator.getReference(),
			pipelineState.getReference(), IID_PPV_ARGS(Commandlist.getInitAddress()) ))) {
			AR_LOG(Error, "*** Create CommandList failed");
		}
		return Commandlist;
	}

	TRefCountPtr<ID3D12Resource> createCommittedResource(D3D12_HEAP_PROPERTIES const& properties, D3D12_HEAP_FLAGS flag,
		D3D12_RESOURCE_DESC const& desc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE* clearValue )
	{
		TRefCountPtr<ID3D12Resource> resource{};
		if( !DX12_ENSURE_SUCC(_device->CreateCommittedResource(&properties, flag, &desc, initialState,
			clearValue, IID_PPV_ARGS(resource.getInitAddress())  )))
		{
			AR_LOG(Error, "*** Create Committed Resource failed");
		}
		return resource;
	}

	TRefCountPtr<ID3D12Fence> createFence(u64 initialValue, D3D12_FENCE_FLAGS flag )
	{
		TRefCountPtr<ID3D12Fence> fence{};
		if( !DX12_ENSURE_SUCC(_device->CreateFence(initialValue, flag, IID_PPV_ARGS(fence.getInitAddress())))){
			AR_LOG(Error, "*** Create Fence failed");
		}
		return fence;
	}

	bool initialize() {

		// auto callback if fail
		ExecuteIfFail FailFn{ [this]() {
			this->finalize();
		}};

		u32 debugFlag = 0;
#if AR_DEBUG
		{
			TRefCountPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(DX12_GET_REF_PTR(debugController))))
			{
				debugController->EnableDebugLayer();
				AR_LOG(Info, "* DX12 Enabled Debug Layer.");

				// Enable additional debug layers.
				debugFlag |= DXGI_CREATE_FACTORY_DEBUG;

				TRefCountPtr<ID3D12Debug1> debugController1;
				if (SUCCEEDED(debugController->QueryInterface(DX12_GET_REF_PTR(debugController1)))) {
					debugController1->SetEnableGPUBasedValidation(true);
					AR_LOG(Info, "* DX12 Enabled GPU-Based Validation.");
				}
				TRefCountPtr<ID3D12Debug2> debugController2;
				if (SUCCEEDED(debugController->QueryInterface(DX12_GET_REF_PTR(debugController2)))) {
					debugController2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
					AR_LOG(Info, "* DX12 Set GPU-Based Validation Flag None.");
				}
				TRefCountPtr<ID3D12Debug5> debugController5;
				if (SUCCEEDED(debugController->QueryInterface(DX12_GET_REF_PTR(debugController5)))) {
					debugController5->SetEnableAutoName(true);
					AR_LOG(Info, "* DX12 Enabled Auto Naming.");
				}
			}
		}
#endif
		if (!DX12_ENSURE_SUCC(CreateDXGIFactory2(debugFlag, IID_PPV_ARGS(_dxgiFactory2.getInitAddress())))) {
			AR_LOG(Info, "* DX12 Create DXGI factory2 failed.");
			return false;
		}
		AR_LOG(Info, "* DX12 Create DXGI factory2 success.");

#define QUERY_DXGI_INTERFACE(Index) \
		if (SUCCEEDED(_dxgiFactory2->QueryInterface(DX12_GET_REF_PTR(_dxgiFactory ## Index)))) { \
			AR_LOG(Info, "* DX12 Get DXGI factory" #Index " success."); \
		}
		QUERY_DXGI_INTERFACE(4)
		QUERY_DXGI_INTERFACE(5)
		QUERY_DXGI_INTERFACE(6)
		QUERY_DXGI_INTERFACE(7)
#undef QUERY_DXGI_INTERFACE


		if (!getHighestFeatureLevelDevice(GWantsFeatureLevels)) {
			AR_LOG(Info, "* DX12 Get Device Failed.");
			return false;
		}
		_nodeCount = _device->GetNodeCount();
		AR_LOG(Info, "* DX12 Get Appropriate Device(with %d Nodes) Success DXGI factory2 success.", _nodeCount);
		if (_nodeCount == 0) {
			return false;
		}

#define QUERY_DEVICE_INTERFACE(Index) \
		if (SUCCEEDED(_device->QueryInterface(DX12_GET_REF_PTR(_device ## Index)))) { \
			AR_LOG(Info, "* DX12 Get Device" #Index " success."); \
		}
		QUERY_DEVICE_INTERFACE(1);
		QUERY_DEVICE_INTERFACE(2);
		QUERY_DEVICE_INTERFACE(3);
		QUERY_DEVICE_INTERFACE(4);
		QUERY_DEVICE_INTERFACE(5);
		QUERY_DEVICE_INTERFACE(6);
		QUERY_DEVICE_INTERFACE(7);
		QUERY_DEVICE_INTERFACE(8);
		QUERY_DEVICE_INTERFACE(9);
		QUERY_DEVICE_INTERFACE(10);
#undef QUERY_DEVICE_INTERFACE
		
		for (i32 i = 0; i < _nodeCount; ++i) {
			_deviceNodes[i] = new DX12DeviceNode(i);
			_deviceNodes[i]->initialize();
		}

		FailFn.setSuccess();
		return true;
	}

	void tick() {

	}

	void finalize() {
		for (i32 i = 0; i < _nodeCount; ++i) {
			if (_deviceNodes[i]) {
				_deviceNodes[i]->finalize();
				delete _deviceNodes[i];
				_deviceNodes[i] = nullptr;
			}
		}
	}


	TRefCountPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC const& queueDesc){
		TRefCountPtr<ID3D12CommandQueue> commandQueue{};
		if (SUCCEEDED(_device->CreateCommandQueue(&queueDesc, DX12_GET_REF_PTR(commandQueue)))) {
			return commandQueue;
		}
		return nullptr;
	}

	ID3D12CommandQueue* GetMainCommandQueue() {
		ARCheck(_deviceNodes[0]);
		return _deviceNodes[0]->GetCommandQueue(EDX12QueueType::Common).getHandle();
	}

	TRefCountPtr<IDXGISwapChain1> createSwapChain1(DXGI_SWAP_CHAIN_DESC1 const& swapchainDesc) {
		TRefCountPtr<IDXGISwapChain1> swapchain{};
		HWND windowHandle = (HWND)GAppConfigs.get<i64>(AppConfigs::WinHandle, 0);
		ARCheck(windowHandle != nullptr);
		if (_dxgiFactory4.isValid() && false) {
			_dxgiFactory4->CreateSwapChainForHwnd(
				GetMainCommandQueue(),        // Swap chain needs the queue so that it can force a flush on it.
				windowHandle,
				&swapchainDesc,
				nullptr,
				nullptr,
				swapchain.getInitAddress()
			);	
		}
		return swapchain;
	}

	TRefCountPtr<IDXGISwapChain> createSwapChain(DXGI_SWAP_CHAIN_DESC& desc)
	{
		TRefCountPtr<IDXGISwapChain> swapchain{};
		HWND windowHandle = (HWND)GAppConfigs.get<i64>(AppConfigs::WinHandle, 0);
		desc.OutputWindow = windowHandle;
		_dxgiFactory2->CreateSwapChain(GetMainCommandQueue(), &desc, swapchain.getInitAddress());
		return swapchain;
	}

	TRefCountPtr<ID3D12Resource> CreateCommittedResource()
	{ 
		D3D12_HEAP_PROPERTIES heapProp;
		// all visible
		heapProp.VisibleNodeMask = nodeMask(_nodeCount);
		// D3D12_MEMORY_POOL_L0 = DXGI_MEMORY_SEGMENT_GROUP_LOCAL, 
		//  discrete GPU has greater bandwidth for the CPU and less bandwidth for the GPU
		//  UMA GPU this pool is the only one which is valid
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CreationNodeMask = nodeMask(0);
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

		D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;

		D3D12_RESOURCE_DESC resourceDesc{};
		
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

		D3D12_CLEAR_VALUE clearValue{};
		TRefCountPtr<ID3D12Resource> Resource;
		if (!SUCCEEDED(_device->CreateCommittedResource(&heapProp, flag, &resourceDesc, state, &clearValue, DX12_GET_REF_PTR(Resource)))) {
			AREnsureFormat(false, "CreateCommittedResource failed");
		}
		return Resource;
	}


};



PROJECT_NAMESPACE_END