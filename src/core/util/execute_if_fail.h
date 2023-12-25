#pragma once

#include"../type.h"

PROJECT_NAMESPACE_BEGIN

struct ExecuteIfFail
{
	bool _success{ false };
	TFunction<void()> _fn;

	template<typename Fn>
	ExecuteIfFail(Fn&& fn)
		:_fn{std::move(fn)}
	{}

	~ExecuteIfFail() {
		if (!_success) {
			_fn();
		}
	}

	void setSuccess() {
		_success = true;
	}
};


PROJECT_NAMESPACE_END