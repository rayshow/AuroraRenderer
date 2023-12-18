
#pragma once

#include"core/util/singleton.h"
#include"core/util/execute_if_fail.h"
#include"dx12_header.h"
#include"dx12_device.h"
 
PROJECT_NAMESPACE_BEGIN

class DX12Viewport: public DX12ContextChild
{
private:
	DX12Viewport() :DX12ContextChild{} {}

public:

	void present();


};

PROJECT_NAMESPACE_END