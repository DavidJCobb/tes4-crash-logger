#include "StackWalk.h"

#pragma comment(lib, "dbghelp")
void StackWalk(EXCEPTION_POINTERS* info) {
    DWORD machine = IMAGE_FILE_MACHINE_I386;

    HANDLE process  = GetCurrentProcess();
    HANDLE thread   = GetCurrentThread();
    CONTEXT context = {};
    memcpy(&context, info->ContextRecord, sizeof(CONTEXT));
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);
    SymSetExtendedOption((IMAGEHLP_EXTENDED_OPTIONS)SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    if (SymInitialize(process, NULL, TRUE) != TRUE) {
        _MESSAGE("Error initializing symbol store");
    }
    SymSetExtendedOption((IMAGEHLP_EXTENDED_OPTIONS)SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);

    STACKFRAME frame = {};
    frame.AddrPC.Offset    = info->ContextRecord->Eip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = info->ContextRecord->Ebp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = info->ContextRecord->Esp;
    frame.AddrStack.Mode   = AddrModeFlat;
    while (StackWalk(machine, process, thread, &frame, &context , NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) {
        std::string functioName;
        char symbolBuffer[sizeof(PSYMBOL_INFO) + 255];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(PSYMBOL_INFO);
        symbol->MaxNameLen = 254;
        DWORD64  offset = 0;
        IMAGEHLP_MODULE module = { 0 };
        module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
        if (SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol)) {
            functioName = symbol->Name;
            if (!SymGetModuleInfo(process, frame.AddrPC.Offset, &module)) {
                _MESSAGE("0x%08X ==> %s+0x%0X in No Module (0x%08X) ", frame.AddrPC.Offset, functioName.c_str(), (DWORD)offset, frame.AddrFrame.Offset);
            }
            else {
                _MESSAGE("0x%08X ==> %s+0x%0X in %s (0x%08X) ", frame.AddrPC.Offset, functioName.c_str(), (DWORD)offset, module.ModuleName, frame.AddrFrame.Offset);
            }
        }
        else {
            _MESSAGE("0x%08X ==> ¯\\(°_o)/¯ (Corrupt stack or heap?)  (0x%08X)", frame.AddrPC.Offset, frame.AddrFrame.Offset);
        }
    }
    SymCleanup(process);
}
