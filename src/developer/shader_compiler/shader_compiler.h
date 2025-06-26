#pragma once

#include"core/type.h"
#include"core/util/enum_as_flag.h"
#include "HAL/logger.h"
#include "render_core/shader.h"

PROJECT_NAMESPACE_BEGIN


enum class EShaderCompileFlag {
    None = 0,
    Debug = 1,
};
ENUM_CLASS_FLAGS(EShaderCompileFlag);

struct ShaderMetaData
{
    String             classPath{};
    String             sourcePath{};
    String             entryPoint{};
    String             target{};
    EShaderStage       stage{ EShaderStage::NumShaderStages };
    EShaderCompileFlag compileFlag{ EShaderCompileFlag::Debug };
};


class ShaderTypeMap
{
    using iterator = typename TMap< String, ShaderMetaData>::iterator;
private:
    TMap< String, ShaderMetaData> _shaderMetaDatas;
    std::string _sourceDir{};
    std::string _destDir{};
public:
    void add(String&& name,ShaderMetaData&& metaData);
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
        auto it = _mapData.find(T::GetShaderName() );
        ARCheck(it != _mapData.end());
        return it->second;
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
    ShaderTypeInitializer(String&& name, String&& classpath, String&& sourcePath, String&& entryPoint, String&& target, EShaderStage stage,  EShaderCompileFlag flag)
    {
        ShaderTypeMap::Instance().add(  std::move(name), ShaderMetaData{ std::move(classpath), std::move(sourcePath), std::move(entryPoint), std::move(target), stage, flag  });
    }
};

class ShaderCompiler
{
private:
    
public:
    void initialize();
};


#define AR_DECLARE_GLOBAL_SHADER(Class)             \
    public:                                         \
    static String& GetShaderName() {                \
        static String ShaderName{ _WIDE( #Class) }; \
        return ShaderName;                  \
    }

#define AR_IMPLEMENTS_GLOBAL_SHADER(Class, FileName, EntryPoint, ShaderStage,  CompileFlag ) \
    static ShaderTypeInitializer<Class> Class ## Test( String{L ## #Class}, String{AR_WIDE_FILE}, String{WIDE_PROJECT_SOURCE_DIR}+ L"/"+ FileName, String{EntryPoint}, String{L""} , ShaderStage, CompileFlag)


PROJECT_NAMESPACE_END


template<>
struct std::hash<ar3d::ShaderMetaData>
{
    size_t operator()(const ar3d::ShaderMetaData& metaData) const noexcept
    {
        return std::hash<ar3d::String>{}(metaData.classPath) ^
            std::hash<ar3d::String>{}(metaData.sourcePath) ^
            std::hash<ar3d::String>{}(metaData.entryPoint) ^
            std::hash<ar3d::String>{}(metaData.target) ^
            std::hash<ar3d::u32>{}(static_cast<ar3d::u32>(metaData.compileFlag)) ^
            std::hash<ar3d::u32>{}(static_cast<ar3d::u32>(metaData.stage));
    }
};