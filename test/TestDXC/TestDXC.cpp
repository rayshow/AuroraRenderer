#include<cstdio>
#include<Windows.h>
#include"dxcapi.h"
#include"HAL/assert.h"
#include"core/type.h"
#include"HAL/filesystem.h"
#include"CLI11.hpp"
using namespace std;

/* #undef CMAKE_SHARED_LIBRARY_PREFIX */
#define CMAKE_SHARED_LIBRARY_SUFFIX ".dll"

#ifndef CMAKE_SHARED_LIBRARY_PREFIX
#define CMAKE_SHARED_LIBRARY_PREFIX
#endif

#ifndef CMAKE_SHARED_LIBRARY_SUFFIX
#define CMAKE_SHARED_LIBRARY_SUFFIX
#endif

const char* kDxCompilerLib =
CMAKE_SHARED_LIBRARY_PREFIX "dxcompiler" CMAKE_SHARED_LIBRARY_SUFFIX;
const char* kDxilLib =
CMAKE_SHARED_LIBRARY_PREFIX "dxil" CMAKE_SHARED_LIBRARY_SUFFIX;


// Helper class to dynamically load the dxcompiler or a compatible libraries.
class DxcDllSupport {
protected:
    HMODULE m_dll;
    DxcCreateInstanceProc m_createFn;
    DxcCreateInstance2Proc m_createFn2;

    HRESULT InitializeInternal(LPCSTR dllName, LPCSTR fnName) {
        if (m_dll != nullptr)
            return S_OK;

#ifdef _WIN32
        m_dll = LoadLibraryA(dllName);
        if (m_dll == nullptr)
            return HRESULT_FROM_WIN32(GetLastError());
        m_createFn = (DxcCreateInstanceProc)GetProcAddress(m_dll, fnName);

        if (m_createFn == nullptr) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            FreeLibrary(m_dll);
            m_dll = nullptr;
            return hr;
        }
#else
        m_dll = ::dlopen(dllName, RTLD_LAZY);
        if (m_dll == nullptr)
            return E_FAIL;
        m_createFn = (DxcCreateInstanceProc)::dlsym(m_dll, fnName);

        if (m_createFn == nullptr) {
            ::dlclose(m_dll);
            m_dll = nullptr;
            return E_FAIL;
        }
#endif

        // Only basic functions used to avoid requiring additional headers.
        m_createFn2 = nullptr;
        char fnName2[128];
        size_t s = strlen(fnName);
        if (s < sizeof(fnName2) - 2) {
            memcpy(fnName2, fnName, s);
            fnName2[s] = '2';
            fnName2[s + 1] = '\0';
#ifdef _WIN32
            m_createFn2 = (DxcCreateInstance2Proc)GetProcAddress(m_dll, fnName2);
#else
            m_createFn2 = (DxcCreateInstance2Proc)::dlsym(m_dll, fnName2);
#endif
        }

        return S_OK;
    }

public:
    DxcDllSupport() : m_dll(nullptr), m_createFn(nullptr), m_createFn2(nullptr) {}

    DxcDllSupport(DxcDllSupport&& other) {
        m_dll = other.m_dll;
        other.m_dll = nullptr;
        m_createFn = other.m_createFn;
        other.m_createFn = nullptr;
        m_createFn2 = other.m_createFn2;
        other.m_createFn2 = nullptr;
    }

    ~DxcDllSupport() { Cleanup(); }

    HRESULT Initialize() {
        return InitializeInternal(kDxCompilerLib, "DxcCreateInstance");
    }

    HRESULT InitializeForDll(LPCSTR dll, LPCSTR entryPoint) {
        return InitializeInternal(dll, entryPoint);
    }

    template <typename TInterface>
    HRESULT CreateInstance(REFCLSID clsid, TInterface** pResult) {
        return CreateInstance(clsid, __uuidof(TInterface), (IUnknown**)pResult);
    }

    HRESULT CreateInstance(REFCLSID clsid, REFIID riid, IUnknown** pResult) {
        if (pResult == nullptr)
            return E_POINTER;
        if (m_dll == nullptr)
            return E_FAIL;
        HRESULT hr = m_createFn(clsid, riid, (LPVOID*)pResult);
        return hr;
    }

    template <typename TInterface>
    HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID clsid,
        TInterface** pResult) {
        return CreateInstance2(pMalloc, clsid, __uuidof(TInterface),
            (IUnknown**)pResult);
    }

    HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID clsid, REFIID riid,
        IUnknown** pResult) {
        if (pResult == nullptr)
            return E_POINTER;
        if (m_dll == nullptr)
            return E_FAIL;
        if (m_createFn2 == nullptr)
            return E_FAIL;
        HRESULT hr = m_createFn2(pMalloc, clsid, riid, (LPVOID*)pResult);
        return hr;
    }

    bool HasCreateWithMalloc() const { return m_createFn2 != nullptr; }

    bool IsEnabled() const { return m_dll != nullptr; }

    void Cleanup() {
        if (m_dll != nullptr) {
            m_createFn = nullptr;
            m_createFn2 = nullptr;
#ifdef _WIN32
            FreeLibrary(m_dll);
#else
            ::dlclose(m_dll);
#endif
            m_dll = nullptr;
        }
    }

    HMODULE Detach() {
        HMODULE hModule = m_dll;
        m_dll = nullptr;
        return hModule;
    }
};

#define CheckRetSucc(Expr) ARCheck( (Expr) >=0 )


using namespace ar3d;
String ExtractBlobMessage(IDxcBlob* pBlob ) {
    if (pBlob == nullptr) {
        return String{ L"Empty Blob" };
    }

    // Try to get as UTF-16 or UTF-8
    BOOL known;
    UINT32 cp = 0;
    TRefCountPtr<IDxcBlobEncoding> pBlobEncoding;
    CheckRetSucc(pBlob->QueryInterface(&pBlobEncoding));
    CheckRetSucc(pBlobEncoding->GetEncoding(&known, &cp));

    if (cp == DXC_CP_WIDE) {
        TRefCountPtr<IDxcBlobWide> pWide;
        if (pBlobEncoding->QueryInterface(&pWide) >= 0)
        {
            String message{ pWide->GetStringPointer(), pWide->GetBufferSize() };
            return message;
        }
    }
    else if (cp == CP_UTF8) {
        TRefCountPtr<IDxcBlobUtf8> pUtf8;
        if (pBlobEncoding->QueryInterface(&pUtf8) >=0)
        {
            AString Utf8Message{ pUtf8->GetStringPointer(), pUtf8->GetBufferSize() };
            return String{ Utf8Message };
        }
        
    }
    return String{ L"Unkown" };
}



int main(int argc, char* argv[])
{
    ARCheck(Logger::initialize(L".\\1.txt"));

    CLI::App app{ "App description" };
    argv = app.ensure_utf8(argv);

    AString inputFileName = "";
    AString outputFileName = "output.hlsl";
    app.add_option("-i,--input", inputFileName, "the input file name");
    app.add_option("-o,--output", outputFileName, "the output file name");
    CLI11_PARSE(app, argc, argv);

    if (inputFileName.size() == 0)
    {
        std::string Help = app.help();
        AR_LOG(Info, "no input filename.\n%s", Help.c_str());
        return 0;
    }


    DxcDllSupport DxcSupport{};
    CheckRetSucc(DxcSupport.Initialize());

    TRefCountPtr<IDxcLibrary> pLibrary;
    TRefCountPtr<IDxcIncludeHandler> pIncludeHandler;
    TRefCountPtr<IDxcCompiler> pCompiler;
    TRefCountPtr<IDxcOperationResult> pPreprocessResult;
    TRefCountPtr<IDxcBlobEncoding> pSource;

    CheckRetSucc(DxcSupport.CreateInstance(CLSID_DxcLibrary, pLibrary.getInitAddress()));
    CheckRetSucc(pLibrary->CreateIncludeHandler(pIncludeHandler.getInitAddress()));
    CheckRetSucc(DxcSupport.CreateInstance(CLSID_DxcCompiler, pCompiler.getInitAddress()));


    String wideInputfileName{ inputFileName };
    String wideOutputfileName{ outputFileName };

    CheckRetSucc(pLibrary->CreateBlobFromFile(wideInputfileName.c_str(), nullptr, pSource.getInitAddress()));

    DxcDefine testDefine{};
    testDefine.Name = L"TEST_MACRO";
    testDefine.Value = L"2.0f";

    TArray<const wchar*> Args{};
    Args.emplace_back(L"-T vs_6_6");
    Args.emplace_back(L"-E VSMain");

    CheckRetSucc(pCompiler->Preprocess(pSource.getReference(), wideInputfileName.c_str(),
        Args.data(), Args.size(), &testDefine, 1, pIncludeHandler,
        pPreprocessResult.getInitAddress()));

    HRESULT status{};
    pPreprocessResult->GetStatus(&status);

    if (status < 0)
    {
        TRefCountPtr< IDxcBlobEncoding> ErrorBlob{};
        if (pPreprocessResult->GetErrorBuffer(&ErrorBlob) >= 0)
        {
            String ErrorMsg = ExtractBlobMessage(ErrorBlob);
            AR_LOG(Info, "open output file failed bacause:%s", ErrorMsg.c_str());
        }
        return -1;
    }
    
    File file{};
    if (file.open(outputFileName, EFileOption::Write | EFileOption::CreateIfNoExists))
    {
        TRefCountPtr< IDxcBlob> Blob{};
        pPreprocessResult->GetResult(&Blob);
        file.rawWrite(Blob->GetBufferPointer(), Blob->GetBufferSize());
    }
    else {
        TLocalBuffer<char, 256> errorBuffer{};
        AR_LOG(Info, "open output file failed bacause:%s", file.getError(errorBuffer.getBuffer(), errorBuffer.Length()));
    }

    TRefCountPtr<IDxcOperationResult> compileResult;
    CheckRetSucc(pCompiler->Compile(pSource.getReference(), wideInputfileName.c_str(), L"VSMain", L"vs_6_6", Args.data(), Args.size(), &testDefine, 1, pIncludeHandler, &compileResult));

    compileResult->GetStatus(&status);

    if (status < 0)
    {
        TRefCountPtr< IDxcBlobEncoding> ErrorBlob{};
        if (compileResult->GetErrorBuffer(&ErrorBlob) >= 0)
        {
            String ErrorMsg = ExtractBlobMessage(ErrorBlob);
            AR_LOG(Info, "compile failed bacause:%ls", ErrorMsg.c_str());
        }
        return -1;
    }
    //IDxcCompiler3
        
    File compileFile{};
    wideOutputfileName.Append(L".dxil");
    if (compileFile.open(wideOutputfileName, EFileOption::Write | EFileOption::CreateIfNoExists))
    {
        TRefCountPtr< IDxcResult> result;
        CheckRetSucc(compileResult->QueryInterface(&result));


        TRefCountPtr< IDxcBlob> Blob{};
        compileResult->GetResult(&Blob);
        compileFile.rawWrite(Blob->GetBufferPointer(), Blob->GetBufferSize());
    }
    else {
        TLocalBuffer<char, 256> errorBuffer{};
        AR_LOG(Info, "open output file failed bacause:%s", file.getError(errorBuffer));
    }


	return 0;
}