#include "ShaderCompiler.h"

PROJECT_NAMESPACE_BEGIN

void ShaderTypeMap::add(String const& name, ShaderMetaData&& metaData) {
    auto iter = _shaderMetaDatas.find( name );
    // exists
    if(iter!=_shaderMetaDatas.end()) {
        AR_LOG(Fatal, "Shader type %ls already exists!", iter->second._path.c_str());
        return;
    } else {
        _shaderMetaDatas[metaData._path] = std::move(metaData);
    }
}

ShaderTypeMap& ShaderTypeMap::instance()
{
    static ShaderTypeMap GShaderTypeMap{};
    return GShaderTypeMap;
}

void ShaderCompiler::initialize()
{
    for(auto item : ShaderTypeMap::instance() )
    {
        auto& path = item.second._path;
        Path sourceFilePath( static_cast<std::wstring>( item.second._sourcePath) );
        
        
        AR_LOG(Info, "item :%s path:%s", item.first.c_str(), path.c_str());

        
        
    }
}

PROJECT_NAMESPACE_END