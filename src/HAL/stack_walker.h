#pragma once
#include"core/compile.h"

#if RS_PLATFORM_ANDROID
#include"android/android_stack_walker.h"
#elif RS_PLATFORM_WINDOW
#else 
#error "unkown platform stack walker"
#endif 

PROJECT_NAMESPACE_BEGIN
    template<i32 N>
    inline void  GetCallStack(achar (&outCallStack)[N])
    {
       // memset(outCallStack,0, N);
        //StackWalker::stackWalkAndDump(outCallStack, N, 0, NULL);
    }
PROJECT_NAMESPACE_END
