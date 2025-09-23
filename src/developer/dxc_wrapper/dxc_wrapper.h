#pragma once

#include "core/type.h"

PROJECT_NAMESPACE_BEGIN

struct DllHandle: public FRefCountedObject
{
    void* handle = nullptr;
    DllHandle(const String& filename);
    virtual ~DllHandle() override;
};

struct DXCWrapper
{
    DXCWrapper();
    ~DXCWrapper();
};

struct DxcMacroDefines
{
    TArray<StringView> macros{};
    TArray<StringView> values{};
    
    void add(const String& macro, const String& value);
    void add(String&& macro,  String&& value);
};

struct DxcArgs
{
    TArray<String> args{};

    void add(String const& arg);
    void add(String&& arg);
};


struct DxcCompileInput
{
    bool bDebugInfo{ false };
    StringView sourceContent;
    StringView inputFileName;
    TArray<StringView> args{};
    TArray<StringView> macros{};
    TArray<StringView> values{};

};

struct DxcCompileOutput
{
    bool bErrorHappened{ false };
    String preprocessed;
    String error;
    ByteBuffer compiledCode;
    ByteBuffer pdb;
};


struct DxcWrapper
{
    static bool preprecess(const DxcCompileInput& input, DxcCompileOutput& output);

    static bool compile(const DxcCompileInput& input, DxcCompileOutput& output);
};

PROJECT_NAMESPACE_END