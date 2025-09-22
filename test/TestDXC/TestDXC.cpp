#include<cstdio>
#include<Windows.h>
#include"dxcapi.h"
#include"HAL/assert.h"
#include"core/type.h"
#include"core/util/refcount.h"
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

using namespace ar3d;
int main(int argc, char* argv[])
{
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


#define CheckRetSucc(Expr) ARCheck( (Expr) >=0 )

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


    File file{};
    if (file.open(outputFileName, EFileOption::Write))
    {
        file.rawWrite(pSource->GetBufferPointer(), pSource->GetBufferSize());
    }

	return 0;
}