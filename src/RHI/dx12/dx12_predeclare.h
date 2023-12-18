#pragma once

PROJECT_NAMESPACE_BEGIN

class DX12Context;
struct DX12ContextChild
{
protected:
	DX12Context& _context;
	DX12ContextChild();
};

PROJECT_NAMESPACE_END