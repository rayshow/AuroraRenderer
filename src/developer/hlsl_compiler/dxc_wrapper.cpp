#include"dxc_wrapper.h"

#if AR_PLATFORM_WINDOW
#include"windows.h"
#endif

#include"core/type.h"

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

PROJECT_NAMESPACE_END