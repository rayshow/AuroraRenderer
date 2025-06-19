#pragma once

#include"core/type.h"
#include "HAL/logger.h"
#include "render_core/shader.h"

PROJECT_NAMESPACE_BEGIN

struct ShaderMetaData
{
    String _sourcePath{};
    String _path{};
    String _entryPoint{};
    String _targetPath{};
    u32 flag{};
};

class ShaderTypeMap
{
    using iterator = typename TMap< String, ShaderMetaData>::iterator;
private:
    TMap< String, ShaderMetaData> _shaderMetaDatas;
    std::string sourceDir{};
    std::string destDir{};
public:
    void add(String const& name,ShaderMetaData&& metaData);
    static ShaderTypeMap& Instance();

    iterator begin(){ return _shaderMetaDatas.begin(); }
    iterator end(){ return _shaderMetaDatas.end(); }
    bool compile();
};

class ShaderMap
{
    using iterator = TMap< String, Shader>;
private:
    TMap< String, Shader> _mapData{};
public:
    ShaderMap() = default;

    void add(String&& name, Shader&& shader);

    template<typename T>
    Shader& getShaderChecked() {
        auto it = _mapData.find(T::Name());
        ARCheck(it != _mapData.end());
        return *it;
    }

    template<typename T>
    static Shader& GetGlobalShader() {
        return GlobalMapInstance().getShaderChecked<T>();
    }
    static ShaderMap& GlobalMapInstance();
};

template<typename T>
struct  ShaderTypeInitializer
{
    ShaderTypeInitializer(String const& name, String const& path, String const& filename, String const& entryPoint, String const& target, u32 flag)
    {
        ShaderTypeMap::Instance().add(  name, ShaderMetaData{ path, filename, entryPoint, target, flag  });
    }
};

class ShaderCompiler
{
private:
    
public:
    void initialize();
};


#define AR_DECLARE_GLOBAL_SHADER(Class)

#define AR_GLOBAL_SHADER(Class, FileName, EntryPoint, Target, Flag ) \
    static ShaderTypeInitializer<Class> Class ## Test( L ## #Class, AR_WIDE_FILE, String{WIDE_PROJECT_SOURCE_DIR}+ L"/"+ FileName, EntryPoint, Target, Flag)


PROJECT_NAMESPACE_END


template<>
struct std::hash<ar3d::ShaderMetaData>
{
    size_t operator()(const ar3d::ShaderMetaData& metaData) const noexcept
    {
        return std::hash<ar3d::String>{}(metaData._path) ^
            std::hash<ar3d::String>{}(metaData._entryPoint) ^
                std::hash<ar3d::String>{}(metaData._targetPath) ^
                    std::hash<ar3d::u32>{}(metaData.flag);
    }
};