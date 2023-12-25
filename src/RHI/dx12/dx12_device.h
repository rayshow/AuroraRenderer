#pragma once

#include"dx12_header.h"
#include"dx12_predeclare.h"

PROJECT_NAMESPACE_BEGIN

class DX12Context;

class D12SyncPoint
{

};

class DX12CommandList 
{
private:
	TRefCountPtr<ID3D12CommandList> CommandList;
public:

};

enum class EDX12QueueType
{
	Common = 0,
	Compute,
	Transfer,
	Tile,
	RenderCount = Tile,
	VideoEncode,
	VideoProcess,
	VideoDecode,
	Count,
};

D3D12_COMMAND_LIST_TYPE DX12GetCommandListType(EDX12QueueType type)
{
	switch (type)
	{
	case ar3d::EDX12QueueType::Common:
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case ar3d::EDX12QueueType::Compute:
		return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case ar3d::EDX12QueueType::Transfer:
		return D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	case ar3d::EDX12QueueType::VideoEncode:
		return D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE;
		break;
	case ar3d::EDX12QueueType::VideoProcess:
		return D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS;
		break;
	case ar3d::EDX12QueueType::VideoDecode:
		return D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
		break;
		break;
	default:
		break;
	}
	return D3D12_COMMAND_LIST_TYPE_DIRECT;
}

class DX12CommandQueue
{
private:
	TRefCountPtr<ID3D12CommandQueue> _queue{};

public:

	bool isValid() const { return _queue.isValid();  }
	ID3D12CommandQueue* getHandle() { return _queue.getReference(); }


	void submit() {
		
	}
};

class DX12DeviceNode : public DX12ContextChild
{
private:
	static constexpr u32 kCount = (u32)EDX12QueueType::RenderCount;
	TStaticArray< DX12CommandQueue, kCount> _mainQueues;
	i32 _nodeIndex;

public:
	DX12DeviceNode(i32 nodeIndex)
		: DX12ContextChild{}
		, _nodeIndex{nodeIndex}
	{}

	AR_FORCEINLINE DX12CommandQueue& GetCommandQueue(EDX12QueueType queueType) {
		i32 index = (i32)queueType;
		ARAssert(index < kCount);
		return _mainQueues[index];
	}

	bool initialize();
	bool finalize();
};

enum class EGPUMask: u8
{
	All = -1,
	Main = 0,
	Aux = 1,
};

class DX12MultiGPUNode
{
	DX12MultiGPUNode* _next{};
};


class DX12Resource: public DX12ContextChild, public DX12MultiGPUNode
{
private:
	EGPUMask _mask;
public:
	DX12Resource(DX12Context& context, EGPUMask mask)
		: DX12ContextChild{}
		, _mask(mask)
	{
		
		switch (_mask)
		{
		case ar3d::EGPUMask::All:
			break;
		case ar3d::EGPUMask::Main:
			break;
		case ar3d::EGPUMask::Aux:
			break;
		default:
			break;
		}
	}
};


PROJECT_NAMESPACE_END