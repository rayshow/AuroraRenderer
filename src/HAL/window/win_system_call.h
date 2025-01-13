#pragma once
#include "core/type.h"
#include<Windows.h>
#include"../../core/type.h"


PROJECT_NAMESPACE_BEGIN

struct WinSystemCall
{
    u32 getLastErrorCode()
    {
        return GetLastError();
    }

    template<class Str, ArCheckType(Str, is_string)>
    Str getLastErrorString() {
        DWORD errorCode = GetLastError();
        DWORD dwChars;
        constexpr u32 kBufferSize = 512;

        if constexpr(std::is_same_v<Str, WString>) {
            wchar   wszMsgBuff[kBufferSize]; 
            dwChars = FormatMessageW(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errorCode,
                0,
                wszMsgBuff,
                kBufferSize,
                NULL);
            return WString{ wszMsgBuff, dwChars };    
        }else {
            char   wszMsgBuff[kBufferSize];  // Buffer for text.
            dwChars = FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errorCode,
                0,
                wszMsgBuff,
                kBufferSize,
                NULL);
            return String{ wszMsgBuff, dwChars };    
        }
    }
  
};

PROJECT_NAMESPACE_END