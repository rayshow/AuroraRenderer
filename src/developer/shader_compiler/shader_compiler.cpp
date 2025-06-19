#include "shader_compiler.h"
#include "HAL/filesystem.h"

#include<ShaderConductor.hpp>

PROJECT_NAMESPACE_BEGIN

void ShaderTypeMap::add(String const& name, ShaderMetaData&& metaData) {
	auto iter = _shaderMetaDatas.find(name);
	// exists
	if (iter != _shaderMetaDatas.end()) {
		AR_LOG(Fatal, "Shader type %ls already exists!", iter->second._path.c_str());
		return;
	}
	else {
		_shaderMetaDatas[metaData._path] = std::move(metaData);
	}
}

void ShaderMap::add(String&& name, Shader&& shader){
	_mapData.emplace(name, shader);
}

ShaderTypeMap& ShaderTypeMap::Instance(){
    static ShaderTypeMap GShaderTypeMap{};
    return GShaderTypeMap;
}

bool ShaderTypeMap::compile() {
    for (auto& pair : _shaderMetaDatas) {
        auto& item = pair.second;
		AR_LOG(Info, "item :%s path:%s", pair.first.c_str(), pair.second._sourcePath.c_str());

		wchar const* sourcePath = item._sourcePath.c_str();
		AString entryPoint{ item._entryPoint };

		char errorMessages[256];
		errorMessages[255] = 0;

		File file{};
		if (!file.open(item._sourcePath, EFileOption::Read)) {
			AR_LOG(Fatal, "failed to open file %s with error", sourcePath, file.getError(errorMessages, 256));
			return false;
		}

        AString buffer{};
		buffer.resize(file.size());
		if(file.readIntoString(buffer) < 0) {
			AR_LOG(Fatal, _WIDE("failed to read file %s error:%s"), sourcePath, file.getError(errorMessages, 256));
			return false;
		}

		ShaderConductor::Compiler::SourceDesc sourceDesc{};
		sourceDesc.stage = ShaderConductor::ShaderStage::VertexShader;
		sourceDesc.entryPoint = "VSMain";
		sourceDesc.fileName = entryPoint.c_str();
		sourceDesc.source = buffer.c_str();

		ShaderConductor::Compiler::Options options{};
		options.disableOptimizations = true;
		options.enable16bitTypes = true;
		options.optimizationLevel = 0;
		options.enableDebugInfo = true;
		options.shaderModel = {6,2};

		ShaderConductor::Compiler::TargetDesc targetDesc{};
		targetDesc.language = ShaderConductor::ShadingLanguage::Dxil;
		targetDesc.version = "vs_6_2";
			
		ShaderConductor::Compiler::ResultDesc result;
		try {
			result = ShaderConductor::Compiler::Compile(sourceDesc, options, targetDesc);
			if (result.hasError) {
				AString errorMsg{ static_cast<const char*>(result.errorWarningMsg.Data()), result.errorWarningMsg.Size() };
				AR_LOG(Error, "shader conductor compile failed:%s", errorMsg.c_str() );
				return false;
			}
		}
		catch (const std::runtime_error e) {
			AR_LOG(Error, "shader conductor compile failed:%s", e.what());
			return false;
		}
		ARCheck(result.target.Size() & 0b11 == 0);

		SHAHash hash = SHAHash::hash(result.target.Data(), result.target.Size());
		TArray<u32> bytes{};
		bytes.resize(result.target.Size() / 4);
		memcpy(bytes.data(), result.target.Data(), result.target.Size());
		Shader shader{hash, std::move(bytes) };
		ShaderMap::GlobalMapInstance().add(String{pair.first}, std::move(shader));
    }
    return true;
}

ShaderMap& ShaderMap::GlobalMapInstance() {
	static ShaderMap Instance{};
	return Instance;
}

void ShaderCompiler::initialize()
{
	ShaderTypeMap::Instance().compile();
}



PROJECT_NAMESPACE_END