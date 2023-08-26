#pragma once

#include"../../type.h"

#include <unwind.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <stdio.h>

PROJECT_NAMESPACE_BEGIN

    struct ProgramCounterSymbolInfo
    {
        static constexpr int32 kMaxNameLength=1024;

        /** Module name. */
        achar	moduleName[kMaxNameLength]{};
        /** Function name. */
        achar	functionName[kMaxNameLength]{};
        /** filename. */
        achar	filename[kMaxNameLength]{};
        /** Line number in file. */
        int32	lineNumber=0;
        /** Symbol displacement of address.	*/
        int32	symbolDisplacement=0;
        /** Program counter offset into module. */
        uint64	offsetInModule =0;
        /** Program counter. */
        uint64	programCounter =0;
    };

    struct GenericCrashContext{

    };

    struct AndroidStackWalker
    {
        inline static uint64* _backTrace = nullptr;
        inline static uint32 _maxDepth = 0;

        static bool symbolInfoToReadableString(const ProgramCounterSymbolInfo& inSymbolInfo, achar* readableString, ptr::size_t readableStringLength){
            constexpr int32 kMaxTempSprintf = 1024;
            constexpr int32 kMaxSprintf=1024;

            //
            // Callstack lines should be written in this standard format
            //
            //	0xaddress module!func [file]
            // 
            // E.g. 0x045C8D01 (0x00009034) OrionClient.self!UEngine::PerformError() [D:\Epic\Orion\Engine\Source\Runtime\Engine\Private\UnrealEngine.cpp:6481]
            //
            // Module may be omitted, everything else should be present, or substituted with a string that conforms to the expected type
            //
            // E.g 0x00000000 (0x00000000) UnknownFunction []
            //
            // 
            if (readableString && readableStringLength > 0) {
                achar stackLine[kMaxSprintf] = { 0 };

                // Strip module path.
                const achar* pos0 = strrchr(inSymbolInfo.moduleName, '\\');
                const achar* pos1 = strrchr(inSymbolInfo.moduleName, '/');
                const achar* realPos = (ptr::uintptr)pos0>(ptr::uintptr)pos1?pos0:pos1;
                const achar* strippedModuleName = (ptr::uintptr)realPos > 0 ?  (realPos + 1) : inSymbolInfo.moduleName;

                // Start with address
                achar pcAddr[kMaxTempSprintf] = { 0 };
                snprintf(pcAddr, kMaxTempSprintf, "0x%016llX ", inSymbolInfo.programCounter);
                strncat(stackLine, pcAddr, kMaxSprintf);
                snprintf(pcAddr, kMaxTempSprintf, "(0x%016llX) ", inSymbolInfo.offsetInModule);
                strncat(stackLine, pcAddr, kMaxSprintf);

                // Module if it's present
                const bool bHasValidModuleName = strlen(strippedModuleName) > 0;
                if (bHasValidModuleName) {
                    strncat(stackLine, strippedModuleName, kMaxSprintf);
                    strncat(stackLine, "!", kMaxSprintf);
                }

                // Function if it's available, unknown if it's not
                const bool bHasValidFunctionName = strlen(inSymbolInfo.functionName) > 0;
                if (bHasValidFunctionName) {
                    strncat(stackLine, inSymbolInfo.functionName, kMaxSprintf);
                }
                else {
                    strncat(stackLine, "UnknownFunction", kMaxSprintf);
                }

                // file info
                const bool bHasValidFilename = strlen(inSymbolInfo.filename) > 0 && inSymbolInfo.lineNumber > 0;
                if (bHasValidFilename)  {
                    achar FilenameAndLineNumber[kMaxTempSprintf] = { 0 };
                    snprintf(FilenameAndLineNumber, kMaxTempSprintf, " [%s:%i]", inSymbolInfo.filename, inSymbolInfo.lineNumber);
                    strncat(stackLine, FilenameAndLineNumber, kMaxSprintf);
                }
                else {
                    strncat(stackLine, " []", kMaxSprintf);
                }

                // Append the stack line.
                strncat(readableString, stackLine, readableStringLength);

                // Return true, if we have a valid function name.
                return bHasValidFunctionName;
            }
            return false;
        }

        static void programCounterToSymbolInfo(uint64 inProgramCounter, ProgramCounterSymbolInfo& outSymbolInfo)
        {
            Dl_info dylibInfo;
            int32 Result = dladdr((const void*)inProgramCounter, &dylibInfo);
            if (Result == 0) {
                return;
            }

            outSymbolInfo.programCounter = inProgramCounter;

            int32 status = 0;
            achar* demangledName = NULL;

            // Increased the size of the demangle destination to reduce the chances that abi::__cxa_demangle will allocate
            // this causes the app to hang as malloc isn't signal handler safe. Ideally we wouldn't call this function in a handler.
            size_t demangledNameLen = 8192;
            achar DemangledNameBuffer[8192] = { 0 };
            demangledName = abi::__cxa_demangle(dylibInfo.dli_sname, DemangledNameBuffer, &demangledNameLen, &status);

            if (demangledName){
                // C++ function
                sprintf(outSymbolInfo.functionName, "%s ", demangledName);
            }
            else if (dylibInfo.dli_sname)  {
                // C function
                sprintf(outSymbolInfo.functionName, "%s() ", dylibInfo.dli_sname);
            }
            else {
                // Unknown!
                sprintf(outSymbolInfo.functionName, "[Unknown]() ");
            }

            // No line number available.
            // TODO open libUE4.so from the apk and get the DWARF-2 data.
            strcat(outSymbolInfo.filename, "Unknown");
            outSymbolInfo.lineNumber = 0;

            // Offset of the symbol in the module, eg offset into libUE4.so needed for offline addr2line use.
            outSymbolInfo.offsetInModule = inProgramCounter - (uint64)dylibInfo.dli_fbase;

            // Write out Module information.
            achar* DylibPath = (achar*)dylibInfo.dli_fname;
            achar* dylibName = strrchr(DylibPath, '/');
            if (dylibName) {
                dylibName += 1;
            }
            else {
                dylibName = DylibPath;
            }
            strcpy(outSymbolInfo.moduleName, dylibName);
        }

        static _Unwind_Reason_Code backtraceCallback(struct _Unwind_Context* inContext, void* inDepthPtr){
            uint32* depthPtr = (uint32*)inDepthPtr;
            // stop if filled the buffer
            if (*depthPtr >= _maxDepth) {
                return _Unwind_Reason_Code::_URC_END_OF_STACK;
            }

            uint64 ip = (uint64)_Unwind_GetIP(inContext);
            if (ip) {
                _backTrace[*depthPtr] = ip;
                (*depthPtr)++;
            }
            return _Unwind_Reason_Code::_URC_NO_REASON;
        }

        static int32 captureStackBackTrace(uint64* inBackTrace, uint32 inMaxDepth, void* Context = nullptr){
            if (inBackTrace == NULL || inMaxDepth == 0){
                return 0;
            }
            memset(inBackTrace, 0, sizeof(uint64)*inMaxDepth);
            _backTrace = inBackTrace;
            _maxDepth = inMaxDepth;
            uint32 depth = 0;
            _Unwind_Backtrace(backtraceCallback, &depth);
            return depth;
        }

        static bool programCounterToReadableString( int32 currentCallDepth, uint64 programCounter, achar* readableString,
             ptr::size_t readableStringLength, GenericCrashContext* pContext )
        {
            if (readableString && readableStringLength > 0)
            {
                ProgramCounterSymbolInfo symbolInfo;
                programCounterToSymbolInfo( programCounter, symbolInfo );
                return symbolInfoToReadableString( symbolInfo, readableString, readableStringLength );
            }
            return false;
        }

        static void stackWalkAndDump( char* readableString, ptr::size_t readableStringLength, int32 inIgnoreCount=0, void* inContext=nullptr ){
            constexpr int32 kMaxDepth = 100;
            uint64 stackTrace[kMaxDepth];
            memset( stackTrace, 0, kMaxDepth*sizeof(uint64) );

            // If the callstack is for the executing thread, ignore this function and the captureStackBackTrace call below
            if(inContext == nullptr){
                inIgnoreCount += 2;
            }

            // Capture stack backtrace.
            uint32 depth = captureStackBackTrace( stackTrace, kMaxDepth, inContext );

            // Skip the first two entries as they are inside the stack walking code.
            uint32 currentDepth = inIgnoreCount;
            while( currentDepth < depth ){
                programCounterToReadableString( currentDepth, stackTrace[currentDepth], readableString, 
                    readableStringLength, reinterpret_cast< GenericCrashContext* >( inContext ) );
                strncat(readableString,  RS_ANSI_LINE_TERMINATOR, readableStringLength);
                currentDepth++;
            }
        }

    };

    using StackWalker = AndroidStackWalker;
    
PROJECT_NAMESPACE_END


