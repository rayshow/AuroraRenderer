#pragma once
#include "core/type.h"
#include<Windows.h>
#include"../../core/type.h"


PROJECT_NAMESPACE_BEGIN

struct WinSystemCall
{
    static u32 getLastErrorCode()
    {
        return GetLastError();
    }

    template<typename Char>
    static DWORD GetSystemMessage(DWORD messageID, Char* buffer, u32 bufferSize) {
        DWORD cchMsg = 0;
        if constexpr (is_wchar_v<Char>) {
            cchMsg = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,  /* (not used with FORMAT_MESSAGE_FROM_SYSTEM) */
                messageID,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                buffer,
                bufferSize,
                NULL);
        }
        else {
            cchMsg = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,  /* (not used with FORMAT_MESSAGE_FROM_SYSTEM) */
                messageID,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                buffer,
                bufferSize,
                NULL);
        }
        return cchMsg;
    }
};

PROJECT_NAMESPACE_END