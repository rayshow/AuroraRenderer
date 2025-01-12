#pragma once

#include"dx12_header.h"
#include"dx12_context.h"

PROJECT_NAMESPACE_BEGIN

class DX12SwapChain: public DX12ContextChild
{
private:
    TRefCountPtr<IDXGISwapChain> _swapChain{};
	TRefCountPtr<IDXGISwapChain1> _swapChain1{};
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
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
        swapChainDesc1.BufferCount = AlgOps::max2(2u, backBufferCount);
        swapChainDesc1.Width = AlgOps::max2(1u, width);
        swapChainDesc1.Height = AlgOps::max2(1u, height);
        swapChainDesc1.Format = format;
        swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc1.SampleDesc.Count = 1;
        _swapChain1 = _context.createSwapChain1(swapChainDesc1);
        if (!_swapChain1.isValid()) {
            DXGI_SWAP_CHAIN_DESC Desc;
            Desc.BufferCount = swapChainDesc1.BufferCount;
            Desc.BufferUsage = swapChainDesc1.BufferUsage;
            Desc.BufferDesc.Width = swapChainDesc1.Width;
            Desc.BufferDesc.Height = swapChainDesc1.Height;
            Desc.BufferDesc.Format = swapChainDesc1.Format;
            Desc.Flags = swapChainDesc1.Flags;
            Desc.SwapEffect = swapChainDesc1.SwapEffect;
            Desc.Windowed = true;
            Desc.SampleDesc = swapChainDesc1.SampleDesc;
            _swapChain = _context.createSwapChain(Desc);
            DX12_GET_INTERFACE(_swapChain, _swapChain1);
        }
        DX12_GET_INTERFACE(_swapChain1, _swapChain2);
        if (DX12_SUCC_GET_INTERFACE(_swapChain, _swapChain3)) {
            _currentPresentIndex = _swapChain3->GetCurrentBackBufferIndex();
            DX12_GET_INTERFACE(_swapChain, _swapChain4);
        }
        _syncInterval = syncInterval;
        _format = format;
        _width = swapChainDesc1.Width;
        _height = swapChainDesc1.Height;
        _backBufferCount = swapChainDesc1.BufferCount;
        return _swapChain.isValid();
    }

    u32 getCurrentBackBufferIndex() const {
        return _currentPresentIndex;
    }

    TRefCountPtr<ID3D12Resource> getBuffer(u32 index)
    {
       TRefCountPtr<ID3D12Resource> buffer{};
       if( !DX12_ENSURE_SUCC( _swapChain->GetBuffer(index, IID_PPV_ARGS(buffer.getInitAddress())))){
           AR_LOG(Error, "Failed to get buffer from swapchain");
       }
        return buffer;
    }

    void present()
    {
        ARCheck(_swapChain.isValid());
        if(_swapChain1.isValid() && false) {
            _swapChain1->Present1(0, 0, nullptr);    
        }else {
            _swapChain->Present(0, 0);
        }
        if (_swapChain3.isValid()) {
            _currentPresentIndex = _swapChain3->GetCurrentBackBufferIndex();
        } else {
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