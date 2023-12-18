#pragma once

#include"dx12_header.h"
#include"dx12_context.h"

PROJECT_NAMESPACE_BEGIN

class DX12SwapChain: public DX12ContextChild
{
private:
	TRefCountPtr<IDXGISwapChain1> _swapChain{};
public:

    bool isValid() const { return _swapChain.isValid(); }

    bool create(u32 backbufferNum, u32 width, u32 height, DXGI_FORMAT format) {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = AlgOps::max2(2u, backbufferNum);
        swapChainDesc.Width = AlgOps::max2(1u, width);
        swapChainDesc.Height = AlgOps::max2(1u, height);
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        _swapChain = _context.CreateSwapChain(swapChainDesc);
        return _swapChain.isValid();
    }

    void present()
    {
        //_swapChain->Present1()
    }

    void destroy() {
        _swapChain = nullptr;
    }

};

PROJECT_NAMESPACE_END