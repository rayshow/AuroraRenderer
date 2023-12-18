#pragma once

PROJECT_NAMESPACE_BEGIN

class DX12Context;
struct DX12ContextChild
{
	DX12Context& _context;
	DX12ContextChild();
};

PROJECT_NAMESPACE_END