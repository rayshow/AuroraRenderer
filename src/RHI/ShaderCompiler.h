#pragma once

#include"core/type.h"
#include "HAL/logger.h"

PROJECT_NAMESPACE_BEGIN


struct ShaderMetaData
{
    String _sourcePath{};
    String _path{};
    String _entryPoint{};
    String _target{};
    u32 flag{};
};

class ShaderTypeMap
{
    using iterator = typename TMap< String, ShaderMetaData>::iterator;
private:
    TMap< String, ShaderMetaData> _shaderMetaDatas;
public:
    void add(String const& name,ShaderMetaData&& metaData);
    static ShaderTypeMap& instance();

    iterator begin(){ return _shaderMetaDatas.begin(); }
    iterator end(){ return _shaderMetaDatas.end(); }
};

extern ShaderTypeMap GShaderTypeMap;

template<typename T>
struct  ShaderTypeInitializer
{
    ShaderTypeInitializer(String const& name, String const& path, String const& filename, String const& entryPoint, String const& target, u32 flag)
    {
        ShaderTypeMap::instance().add(  name, ShaderMetaData{path, filename, entryPoint, target, flag  });
    }
};

class ShaderCompiler
{
private:
    
public:
    void initialize();
};


#define AR_GLOBAL_SHADER(Class, FileName, EntryPoint, Target, Flag ) \
    static ShaderTypeInitializer<Class> Test( L ## #Class, AR_WIDE_FILE ,  FileName, EntryPoint, Target, Flag)


PROJECT_NAMESPACE_END


template<>
struct std::hash<_AR::ShaderMetaData>
{
    size_t operator()(const _AR::ShaderMetaData& metaData) const noexcept
    {
        return std::hash<_AR::String>{}(metaData._path) ^
            std::hash<_AR::String>{}(metaData._entryPoint) ^
                std::hash<_AR::String>{}(metaData._target) ^
                    std::hash<_AR::u32>{}(metaData.flag);
    }
};