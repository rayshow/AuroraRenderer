#include<string>
#include<vector>
#include<unordered_map>
#include<memory>

#include"HAL/application.h"
#include"core/type.h"
#include "RHI/shader.h"
using namespace _AR;
std::vector<std::string> GCmdLines;

#include"RHI/dx12/dx12_swapchain.h"
#include"RHI/ShaderCompiler.h"
#include"d3dx12.h"
#include<d3dcompiler.h>
#include<DirectXMath.h>
using namespace ar3d;


static const u32 FrameCount = 2;

class FTriangleShader: public Shader
{
	
};


AR_GLOBAL_SHADER(FTriangleShader, _LIT("shaders.hlsl"), _LIT("VSMain"), _LIT("vs_5_0"), D3DCOMPILE_DEBUG);


struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

class DrawRawTriangle : public IAppPlugin
{
public:
	DrawRawTriangle(){}
    virtual ~DrawRawTriangle() {}
 
	virtual EExitCode initialize() override
	{
		DX12Context& Context = DX12Context::getInstance();
		_swapchain.create(FrameCount, 800, 600, true);


		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
 
		_rtvHeap = Context.createDescriptorHeap(rtvHeapDesc);
		_rtvDescriptorSize = Context.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
 
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		// Create a RTV for each frame.
		for (u32 n = 0; n < FrameCount; n++)
		{
			_renderTargets[n] = _swapchain.getBuffer(n);
			Context.createRenderTargetView(_renderTargets[n].getReference(), nullptr, rtvHandle);
			rtvHandle.Offset(1, _rtvDescriptorSize);
		}

		_allocator = Context.createCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);

		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			_rootSignature = Context.createRootSignature(0, rootSignatureDesc );

			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			TRefCountPtr<ID3DBlob> vertexShader;
			TRefCountPtr<ID3DBlob> pixelShader;

			auto& workspacePath = GAppConfigs.get<String>(AppConfigs::BinDir);

			 
			
			String vspath{workspacePath + L"shaders.hlsl"};
			String pspath{workspacePath + L"shaders.hlsl"};

	#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
			UINT compileFlags = 0;
	#endif

			D3DCompileFromFile(vspath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, vertexShader.getInitAddress(), nullptr);
			D3DCompileFromFile(pspath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pixelShader.getInitAddress(), nullptr);
			
			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = _rootSignature.getReference();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.getReference());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.getReference());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			_pipelineState = Context.createGraphicsPipeline(psoDesc);

			_commandList = Context.createCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _allocator, _pipelineState);
			DX12_CHECK_SUCC(_commandList->Close());
		}

		{
			float aspectRatio = 800.0 / 600;
				
			// Define the geometry for a triangle.
			Vertex triangleVertices[] =
			{
				{ { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
				{ { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
				{ { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
			};

			const u32 vertexBufferSize = sizeof(triangleVertices);

			_vertexBuffer = Context.createCommittedResource(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr );

			UINT8* pVertexDataBegin;
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			DX12_ENSURE_SUCC(_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
			_vertexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			m_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
			
		}

		_fence = Context.createFence(0, D3D12_FENCE_FLAG_NONE);
		_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ARCheck(_fenceEvent != nullptr);

		waitPreviousFrame();
		
        printf("DrawRawTriangle::initialize");
		return EExitCode::Success;
	}

	void waitPreviousFrame()
	{
		const u64 fence = _fenceValue;
		DX12_ENSURE_SUCC(DX12Context::getInstance().GetMainCommandQueue()->Signal(_fence.getReference(), fence));
		_fenceValue++;

		// Wait until the previous frame is finished.
		if (_fence->GetCompletedValue() < fence)
		{
			DX12_ENSURE_SUCC(_fence->SetEventOnCompletion(fence, _fenceEvent));
			WaitForSingleObject(_fenceEvent, INFINITE);
		}
		_frameIndex = _swapchain.getCurrentBackBufferIndex();
	}

	void PopulateCommandList()
	{
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		DX12_ENSURE_SUCC(_allocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		DX12_ENSURE_SUCC(_commandList->Reset(_allocator.getReference(), _pipelineState.getReference()));

		// Set necessary state.
		_commandList->SetGraphicsRootSignature(_rootSignature.getReference());

		CD3DX12_VIEWPORT viewport{0.0f,0.0f,800.0f,600.0f};
		CD3DX12_RECT scissorRect{0,0,800,600};
		_commandList->RSSetViewports(1, &viewport);
		_commandList->RSSetScissorRects(1, &scissorRect);

		// Indicate that the back buffer will be used as a render target.
		D3D12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].getReference(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_commandList->ResourceBarrier(1, &transition);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _frameIndex, _rtvDescriptorSize);
		_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// Record commands.
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		_commandList->DrawInstanced(3, 1, 0, 0);

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER transition2 = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].getReference(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		_commandList->ResourceBarrier(1, &transition2);

		DX12_ENSURE_SUCC(_commandList->Close());
	}
	
	virtual void tick(double) override
	{
        //printf("DrawRawTriangle::tick");
	}

	virtual void render(double) override
	{
		// Record all the commands we need to render the scene into the command list.
		PopulateCommandList();

		auto& Context = DX12Context::getInstance();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { _commandList.getReference() };
		Context.GetMainCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		_swapchain.present();
		
		waitPreviousFrame();
	}
	
	virtual void finalize() override
	{
        printf("DrawRawTriangle::finalize");
	}

	DX12SwapChain _swapchain;
	TRefCountPtr<ID3D12RootSignature> _rootSignature{};
	TRefCountPtr<ID3D12PipelineState> _pipelineState{};
	TRefCountPtr<ID3D12DescriptorHeap> _rtvHeap;
	TRefCountPtr<ID3D12CommandAllocator> _allocator;
	TRefCountPtr<ID3D12GraphicsCommandList> _commandList{};
	u64 _rtvDescriptorSize;

	TRefCountPtr<ID3D12Resource> _renderTargets[FrameCount];
	TRefCountPtr<ID3D12Resource> _resource{};
	TRefCountPtr<ID3D12Resource> _vertexBuffer{};

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	HANDLE _fenceEvent;
	TRefCountPtr<ID3D12Fence> _fence;
	u64 _fenceValue;
	u32 _frameIndex;
	
};

AR_REGISTER_PLUGIN(DrawRawTriangle)