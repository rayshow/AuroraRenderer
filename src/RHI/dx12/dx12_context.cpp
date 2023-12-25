#include"dx12_context.h"
#include"dx12_predeclare.h"

PROJECT_NAMESPACE_BEGIN

DX12ContextChild::DX12ContextChild()
	:_context{ DX12Context::getInstance() }
{}


PROJECT_NAMESPACE_END