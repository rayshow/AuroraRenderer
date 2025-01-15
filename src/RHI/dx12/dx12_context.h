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

	u32                                      _nodeCount{ 0 };
	TStaticArray< DX12DeviceNode*, MAX_GPUS> _deviceNodes{};
	u64 _rtvDescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]{};
	
	static constexpr D3D_FEATURE_LEVEL GWantsFeatureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};
	
	bool enumAdapter(DXGI_GPU_PREFERENCE gpuPreference, TRefCountPtr<IDXGIAdapter1>& Adapter);
	bool getHighestFeatureLevelDevice(TArrayView< D3D_FEATURE_LEVEL> const& availableFeatureLevels, DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE );

public:

	i32 nodeMask(i32 index) const {
		return nodeCount() == 1 ? 0 : (1 << (1 + index));
	}

	i32 nodeCount() const {
		return _device->GetNodeCount();
	}

	DX12DeviceNode* getDevice(u32 index){
		ARCheck(index < _deviceNodes.size());
		return _deviceNodes[index];
	}

	u64 getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const {
		return _rtvDescriptorSizes[type];
	}
	
	ID3D12CommandQueue* GetMainCommandQueue() {
		ARCheck(_deviceNodes[0]);
		return _deviceNodes[0]->GetCommandQueue(EDX12QueueType::Common).getHandle();
	}
	
	bool initialize();
	void tick() {}
	void finalize();
	
	TRefCountPtr<ID3D12CommandQueue> createCommandQueue(D3D12_COMMAND_QUEUE_DESC const& queueDesc) const;
	TRefCountPtr<IDXGISwapChain1> createSwapChain1(DXGI_SWAP_CHAIN_DESC1 const& swapchainDesc);
	TRefCountPtr<IDXGISwapChain> createSwapChain(DXGI_SWAP_CHAIN_DESC& desc);
	TRefCountPtr<ID3D12Resource> CreateCommittedResource();
	TRefCountPtr<ID3D12CommandAllocator> createCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
	TRefCountPtr<ID3D12GraphicsCommandList> createCommandList(u32 nodeMask, D3D12_COMMAND_LIST_TYPE type,
		TRefCountPtr<ID3D12CommandAllocator>const & allocator, TRefCountPtr<ID3D12PipelineState> const& pipelineState);
	TRefCountPtr<ID3D12Resource> createCommittedResource(D3D12_HEAP_PROPERTIES const& properties, D3D12_HEAP_FLAGS flag,
		D3D12_RESOURCE_DESC const& desc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE* clearValue );
	TRefCountPtr<ID3D12Fence> createFence(u64 initialValue, D3D12_FENCE_FLAGS flag );
	TRefCountPtr<ID3D12RootSignature> createRootSignature(u32 nodeMask, const void *pBlobWithRootSignature, u64  blobLengthInBytes );
	TRefCountPtr<ID3D12RootSignature> createRootSignature(u32 nodeMask, D3D12_ROOT_SIGNATURE_DESC const &desc);
	TRefCountPtr<ID3D12PipelineState> createGraphicsPipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC const& Desc);
	TRefCountPtr<ID3D12DescriptorHeap> createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC const& Desc);
	void createRenderTargetView(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle ) {
		_device->CreateRenderTargetView(resource, desc, handle);
	}
};



PROJECT_NAMESPACE_END