#pragma once
#include"core/type.h"

#if AR_PLATFORM_ANDROID
#include"android/android_stack_walker.h"
#elif AR_PLATFORM_WINDOW
#else 
#error "unkown platform stack walker"
#endif 

#include"logger.h"

PROJECT_NAMESPACE_BEGIN

template<i32 N>
AR_FORCEINLINE void GetCallStack(char (&outCallStack)[N])
{
    // memset(outCallStack,0, N);
    //StackWalker::stackWalkAndDump(outCallStack, N, 0, NULL);
}

AR_FORCEINLINE void DumpStackFrame()
{
    char callStack[4096];
    GetCallStack(callStack);
    AR_LOG(Info, "Stack(%d):%s", RawStrOps<char>::length(callStack), callStack);
}

PROJECT_NAMESPACE_END
