#pragma once

#include"dx12_header.h"
#include"dx12_context.h"

PROJECT_NAMESPACE_BEGIN

class DX12Resource: public DX12ContextChild
{
private:
	TRefCountPtr< ID3D12Resource> _resource;
public:
	bool create()
	{

	}
};

class DX12Texture
{
private:
	ID3D12Resource
	DX12Texture()
};


PROJECT_NAMESPACE_END