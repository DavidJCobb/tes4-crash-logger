#include "StackWalk.h"

#pragma comment(lib, "dbghelp")
void StackWalk(EXCEPTION_POINTERS* info) {
    DWORD machine = IMAGE_FILE_MACHINE_I386;

    HANDLE process  = GetCurrentProcess();
    HANDLE thread   = GetCurrentThread();
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
    context.Eip = info->ContextRecord->Eip;
    context.Ebp = info->ContextRecord->Ebp;
    context.Esp = info->ContextRecord->Esp;
    //TODO others? Or memset?
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);
    if (SymInitialize(process, NULL, TRUE) != TRUE) {
        _MESSAGE("Error initializing symbol store");
    }
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
        char moduleBuff[MAX_PATH];
        if (SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol)) {
            functioName = symbol->Name;
            if (!(symbol->ModBase && GetModuleFileNameA((HINSTANCE)symbol->ModBase, moduleBuff, MAX_PATH))) {
                strcpy_s(moduleBuff, "No Module");
            }
            _MESSAGE("0x%08X ==> %s  (%s) (0x%08X) ", frame.AddrPC.Offset, functioName.c_str(), moduleBuff, 0);

//            IMAGEHLP_LINE line = { 0 };
//            line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
//            if (SymGetLineFromAddr(process, frame.AddrPC.Offset, 0, &line)) {
//                _MESSAGE("0x%08X ==> %s  (%s) (0x%08X) ", frame.AddrPC.Offset, functioName.c_str(), moduleBuff, 0);
 //           }
 //           else {
//            }
        }
        else {
            _MESSAGE("0x%08X ==> ¯\\(°_o)/¯ (Corrupt stack or heap?)", frame.AddrPC.Offset, moduleBuff);
        }
    }
    SymCleanup(process);
}
