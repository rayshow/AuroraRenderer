#pragma once

#include"dx12_header.h"
#include"dx12_context.h"

PROJECT_NAMESPACE_BEGIN

class DX12SwapChain: public DX12ContextChild
{
private:
	TRefCountPtr<IDXGISwapChain1> _swapChain{};
    TRefCountPtr<IDXGISwapChain2> _swapChain2{};
    TRefCountPtr<IDXGISwapChain3> _swapChain3{};
    TRefCountPtr<IDXGISwapChain4> _swapChain4{};
    
    bool        _syncInterval{ false };
    u32         _width{ 1u };
    u32         _height{ 1u };
    DXGI_FORMAT _format{ DXGI_FORMAT_UNKNOWN };
    u32         _currentPresentIndex{ 0 };
    u32         _backBufferCount{ 0 };
public:

    bool isValid() const { return _swapChain.isValid(); }

    bool create(u32 backBufferCount, u32 width, u32 height,
        bool syncInterval = true ,
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM) 
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = AlgOps::max2(2u, backBufferCount);
        swapChainDesc.Width = AlgOps::max2(1u, width);
        swapChainDesc.Height = AlgOps::max2(1u, height);
        swapChainDesc.Format = format;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        _swapChain = _context.CreateSwapChain(swapChainDesc);
        if (!_swapChain.isValid()) {
            return false;
        }
        DX12_GET_INTERFACE(_swapChain, _swapChain2);
        if (DX12_SUCC_GET_INTERFACE(_swapChain, _swapChain3)) {
            _currentPresentIndex = swapchain3->GetCurrentBackBufferIndex();
            DX12_GET_INTERFACE(_swapChain, _swapChain4);
        }
        _syncInterval = syncInterval;
        _format = format;
        _width = swapChainDesc.Width;
        _height = swapChainDesc.Height;
        _backBufferCount = swapChainDesc.BufferCount;
        return _swapChain.isValid();
    }

    u32 getCurrentBackBufferIndex() {
        return _currentPresentIndex;
    }

    void present()
    {
        ARCheck(_swapChain.isValid());
        _swapChain->Present1(_syncInterval, 0);
        if (_swapChain3.isValid()) {
            _currentPresentIndex = swapchain3->GetCurrentBackBufferIndex();
        }
        else {
            _currentPresentIndex = (_currentPresentIndex + 1) % _backBufferCount;
        }
    }

    void destroy() {
        _swapChain = nullptr;
        _syncInterval = false;
        _format = DXGI_FORMAT_UNKNOWN;
        _width = 1u;
        _height = 1u;
    }

};

PROJECT_NAMESPACE_END