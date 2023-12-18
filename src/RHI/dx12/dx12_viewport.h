
#pragma once

#include"core/util/singleton.h"
#include"core/util/execute_if_fail.h"
#include"dx12_header.h"
#include"dx12_device.h"
 
PROJECT_NAMESPACE_BEGIN

class DX12Viewport
{
private:
	DX12Viewport(){}

public:

	void present();


};

PROJECT_NAMESPACE_END