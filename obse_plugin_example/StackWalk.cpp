#include "StackWalk.h"

#pragma comment(lib, "dbghelp")
void StackWalk(EXCEPTION_POINTERS* info, HANDLE process, HANDLE thread) {
    DWORD machine = IMAGE_FILE_MACHINE_I386;
    CONTEXT context = {};
    memcpy(&context, info->ContextRecord, sizeof(CONTEXT));
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);
//    SymSetExtendedOption((IMAGEHLP_EXTENDED_OPTIONS)SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    if (SymInitialize(process, NULL, TRUE) != TRUE) {
        _MESSAGE("Error initializing symbol store");
    }
//    SymSetExtendedOption((IMAGEHLP_EXTENDED_OPTIONS)SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);

    STACKFRAME frame = {};
    frame.AddrPC.Offset    = info->ContextRecord->Eip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = info->ContextRecord->Ebp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = info->ContextRecord->Esp;
    frame.AddrStack.Mode   = AddrModeFlat;
    DWORD eip = 0;
    while (StackWalk(machine, process, thread, &frame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) {
        /*
    Using  a PDB for OBSE from VS2019 is causing the frame to repeat, but apparently only if WINEDEBUG=+dbghelp isn't setted. Is this a wine issue?
    When this happen winedbg show only the first line (this happens with the first frame only probably, even if there are more frames shown when using WINEDEBUG=+dbghelp )
        */
        if (frame.AddrPC.Offset == eip) break;
        eip = frame.AddrPC.Offset;
        char path[MAX_PATH];
        /*if (GetModuleFileName((HMODULE)frame.AddrPC.Offset, path, MAX_PATH)) {  //Do this work on non base addresses even on  Windows? Cal directly the LDR function?
            if (!SymLoadModule(process, NULL, path, NULL, 0, 0)) _MESSAGE("Porcoddio %0X", GetLastError());
        }*/
        IMAGEHLP_MODULE module = { 0 };
        module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

        BOOL moduleAv = SymGetModuleInfo(process, frame.AddrPC.Offset, &module);
        std::string functioName;
        char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
        IMAGEHLP_SYMBOL* symbol = (IMAGEHLP_SYMBOL*)symbolBuffer;
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        symbol->MaxNameLength = 254;
        DWORD  offset = 0;
        if (moduleAv) {
            if (SymGetSymFromAddr(process, frame.AddrPC.Offset, &offset, symbol)) {
                functioName = symbol->Name;
                _MESSAGE("0x%08X ==> %s+0x%0X in %s (0x%08X) ", frame.AddrPC.Offset, functioName.c_str(), offset, module.ModuleName, frame.AddrFrame.Offset);
            }
            else {
             //   _MESSAGE("porcoddio1 %0X", GetLastError());
                //WIne dbghelp and Windows dbghelp operate totally differently
                _MESSAGE("0x%08X ==> %s+0x%0X in %s (0x%08X) ", frame.AddrPC.Offset, "EntryPoint", (UInt32)-1, module.ModuleName, frame.AddrFrame.Offset);

            }
        }
        else {
        //    _MESSAGE("porcoddio2 %0X", GetLastError());
            _MESSAGE("0x%08X ==> ¯\\(°_o)/¯ (Corrupt stack or heap?)  (0x%08X)", frame.AddrPC.Offset, frame.AddrFrame.Offset);
        }
    }
}
