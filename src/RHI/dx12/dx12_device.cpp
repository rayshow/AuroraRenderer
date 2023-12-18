#include"dx12_device.h"
#include"dx12_context.h"

PROJECT_NAMESPACE_BEGIN

bool DX12Device::initialize()
{
	bool allowTimeOut = true;
	for (i32 i = 0; i < kCount; ++i)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.NodeMask = _context.nodeMask(_nodeIndex);
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Type = DX12GetCommandListType((EDX12QueueType)i);
		queueDesc.Flags = allowTimeOut ? D3D12_COMMAND_QUEUE_FLAG_NONE
			: D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
		_mainQueues[i] = _context.CreateCommandQueue(queueDesc);
		if (!_mainQueues[i].isValid()) {
			if (i == 0) {
				AR_LOG(Info, "DX12 Get Common Command Queue Failed!");
				return false;
			}
			else {
				_mainQueues[i] = _mainQueues[0];
			}
		}
	}
}

void DX12Device::finalize()
{

}

PROJECT_NAMESPACE_END