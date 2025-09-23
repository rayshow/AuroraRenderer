#include"dxc_wrapper.h"

#if AR_PLATFORM_WINDOW
#include"windows.h"
#endif

#include"core/type.h"
#include"dxcapi.h"


PROJECT_NAMESPACE_BEGIN

DllHandle::DllHandle(String const& filename)
{
    if (handle = GetModuleHandleW(filename.c_str()))
    {
        return;
    }
    handle = LoadLibraryW(filename.c_str());
}

DllHandle::~DllHandle()
{
    if(handle)
    {
        ::FreeLibrary((HMODULE)handle);
        handle = nullptr;
    }
}

static TRefCountPtr<DllHandle> GDXILHandle{nullptr};
static TRefCountPtr<DllHandle> GDXCHandle{nullptr};

DXCWrapper::DXCWrapper()
{
    if(GDXILHandle.getRefCount() == 0)
    {
        GDXILHandle = new DllHandle(_WIDE("dxil.dll"));
        GDXCHandle = new DllHandle(_WIDE("dxcompiler.dll"));
    }
    else
    {
        GDXILHandle->addRef();
        GDXCHandle->addRef();
    }
}

DXCWrapper::~DXCWrapper()
{
    GDXILHandle.safeRelease();
    GDXCHandle.safeRelease();
}

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


#define DxcSucc(Expr) (Expr) >=0
#define DxcFailed(expr) (Expr) < 0
#define DxcCheckSucc(Expr) ARCheck(DxcSucc(Expr))


class DxcCompiler
{
    DxcDllSupport dxcDll{};
    TRefCountPtr<IDxcUtils> dxcUtil;
    TRefCountPtr<IDxcIncludeHandler> dxcIncludeHandler;
    TRefCountPtr<IDxcCompiler>  dxcCompiler;
    TRefCountPtr<IDxcCompiler2> dxcCompiler2;
    TRefCountPtr<IDxcCompiler3> dxcCompiler3;
    bool bInitialized{ false };

    DxcCompiler() {}

    bool initialize()
    {
        if (!bInitialized)
        {
            DxcCheckSucc(dxcDll.Initialize());
            DxcCheckSucc(dxcDll.CreateInstance(CLSID_DxcLibrary, dxcUtil.getInitAddress()));
            ArCheck(dxcUtil.isValid());
            DxcCheckSucc(dxcUtil->CreateDefaultIncludeHandler(dxcIncludeHandler.getInitAddress()));
            DxcCheckSucc(dxcDll.CreateInstance(CLSID_DxcCompiler, dxcCompiler.getInitAddress()));
            bInitialized = true;
            DxcCheckSucc(dxcCompiler->QueryInterface(dxcCompiler3));
        }
    }

    TRefCountPtr<IDxcResult> compile(StringView const& sourceContent, WConstRawStr filename, WConstRawStr* macroDefines, u32 macroDefineSize, WConstRawStr* args, u32 argsSize)
    {
        ARCheck(bInitialized && dxcCompiler3.isValid());

        TRefCountPtr<IDxcResult> result{};

        DxcBuffer sourceBuffer{};
        sourceBuffer.Ptr =  sourceContent.data();
        sourceBuffer.Size = sourceContent.size();
        sourceBuffer.Encoding = DXC_CP_UTF16;

        dxcCompiler3->Compile(sourceBuffer, filename, macroDefines, macroDefineSize, args, argsSize, dxcIncludeHandler, IID_PPV_ARGS(result)));
        ARCheck(result->GetNumOutputs() > 0);
        return result;
    }

    TRefCountPtr<IDxcBlobWide> createBlob(const String& Content)
    {
        ARCheck(bInitialized && dxcUtil.isValid());
        TRefCountPtr<IDxcBlobWide> result{};
        dxcUtil->CreateBlobFromPinned(Content.data(), Content.size(), DXC_CP_UTF16, &result);
        return result;
    }

    DxcCompiler& Instance()
    {
        static DxcCompiler sInstance{};
        sInstance.initialize();
        return sInstance;
    }
};

//https://zhuanlan.zhihu.com/p/362715136
DxcWrapper::preprecess(const DxcCompileInput& input, DxcCompileOutput& output)
{
    DxcCompiler& compiler = DxcCompiler::Instance();

    TStaticArray<DxcDefine, 256> defineRefs{};

    ARAssert(input.Macros.size() == input.values.size());

    for (u32 i = 0; i < input.macros.size(); ++i)
    {
        defineRefs[i].Name = input.macros[i].data();
        defineRefs[i].Value = input.values[i].data();
    }

    TStaticArray<WConstRawStr, 128> argRefs{};
    for (u32 i = 0; i < input.args.size(); ++i)
    {
        argRefs[i] = input.args[i].data();
    }

    TRefCountPtr<IDxcResult> result = compiler.compile(input.sourceContent, input.inputFileName,
        defineRefs.data(), input.macros.size(), argRefs.data(), input.args.size());

    HRESULT status{};
    DxcCheckSucc(result->GetStatus(status));

    if(DxcFailed(status))
    {
        TRefCountPtr<IDxcBlobWide> ErrorBlob{};
        if ( DxcSucc(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(ErrorBlob.getInitAddress()) )))
        {
            output.bErrorHappened = true;
            output.error = String{ ErrorBlob->GetStringPointer(), ErrorBlob->GetStringLength() };
            AString Utf8Msg{ output.error };
            AR_LOG(Error, "open output file failed bacause:%s", Utf8Msg.c_str());
        }
        else {
            AR_LOG(Error, "dxc compile failed and get output failed");
        }
        return false;
    }

    TRefCountPtr<IDxcBlobWide> preprocessContent{};
    if (DxcSucc(result->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(preprocessContent.getInitAddress()))))
    {
        output.preprocessed = String{ preprocessContent->GetStringPointer(), preprocessContent->GetStringLength() };
    }

    TRefCountPtr<IDxcBlob> object{};
    if (DxcSucc(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(object.getInitAddress()))))
    {
        output.compiledCode.resize(object->GetBufferSize());
        MemoryOps::copy(output.compiledCode.data(), object->GetBufferPointer(), object->GetBufferSize());
    }
}


PROJECT_NAMESPACE_END