#pragma once

#include<dxgi1_6.h>
#include<d3d12sdklayers.h>
#include<dxgiformat.h>
#include"core/type.h"
#include"core/util/refcount.h"
#include"HAL/logger.h"
#include"HAL/assert.h"

#define DX12_GET_REF_PTR(Object) IID_PPV_ARGS(Object.getInitAddress())
#define DX12_ENSURE_SUCC(Ret) AREnsure(SUCCEEDED(Ret))
#define DX12_CHECK_SUCC(Ret) ARCheck(SUCCEEDED(Ret))

enum class EGPUVender {
	AMD = 0x1002,
	NVidia = 0x10de,
	Intel = 0x8086,
	Apple = 0x106b,
	Microsoft = 0x1414,
};

inline char const* D3DGetGPUVenderString(EGPUVender id) {
	switch (id) {
#define _GPU_CASE(Name) case EGPUVender::Name: return #Name
		_GPU_CASE(AMD);
		_GPU_CASE(NVidia);
		_GPU_CASE(Intel);
		_GPU_CASE(Apple);
		_GPU_CASE(Microsoft);
		default:break;
#undef _GPU_CASE
	}
	return "NoneVender";
}

inline char const* D3DGetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel) {
	switch (featureLevel) {
#define _GPU_CASE(Name) case Name: return #Name
		_GPU_CASE(D3D_FEATURE_LEVEL_12_2);
		_GPU_CASE(D3D_FEATURE_LEVEL_12_1);
		_GPU_CASE(D3D_FEATURE_LEVEL_12_0);
		_GPU_CASE(D3D_FEATURE_LEVEL_11_1);
		_GPU_CASE(D3D_FEATURE_LEVEL_11_0);
		default:break;
#undef _GPU_CASE
	}
	return "NoneFeatureLevel";
}

