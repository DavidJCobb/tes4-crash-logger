#include "CrashLog.h"
#include "obse_common/SafeWrite.h"

namespace CobbPatches {
   namespace CrashLog {
      static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI s_originalFilter = nullptr;
      //
      bool _AddressIsProbablyVanillaCode(UInt32 a) { // there are better checks but those require walking all loaded modules
         return (a & 0x00FFFFFF) == a && a > 0x001FFFFF;
      }
      //
      void Log(EXCEPTION_POINTERS* info) {
         _MESSAGE("Exception caught!");
         UInt32 eip = info->ContextRecord->Eip;
         _MESSAGE("Instruction pointer (EIP): %08X", eip);
         _MESSAGE("\nREG | VALUE");
         _MESSAGE("%s | %08X", "eax", info->ContextRecord->Eax);
         _MESSAGE("%s | %08X", "ebx", info->ContextRecord->Ebx);
         _MESSAGE("%s | %08X", "ecx", info->ContextRecord->Ecx);
         _MESSAGE("%s | %08X", "edx", info->ContextRecord->Edx);
         _MESSAGE("%s | %08X", "edi", info->ContextRecord->Edi);
         _MESSAGE("%s | %08X", "esi", info->ContextRecord->Esi);
         _MESSAGE("%s | %08X", "ebp", info->ContextRecord->Ebp);
         _MESSAGE("\nVANILLA CALL STACK at %08X:", info->ContextRecord->Esp);
         {
            constexpr UInt32 levels = 20;
            //
            std::string stack;
            stack.reserve(9 * levels);
            UInt32* esp = (UInt32*) info->ContextRecord->Esp;
            try {
               UInt32 i = 0;
               char   s[9];
               do {
                  UInt32 p = esp[i];
                  if (_AddressIsProbablyVanillaCode(p)) {
                     sprintf(s, "%08X", p);
                     stack += s;
                     stack += ' ';
                  }
               } while (++i < levels);
               _MESSAGE("   %s", stack.c_str());
            } catch (...) {
               _MESSAGE("   FAILED TO PRINT.");
            }
         }
         _MESSAGE("\nFULL CALL STACK:", info->ContextRecord->Esp);
         {
            constexpr UInt32 levels = 30;
            //
            std::string stack;
            stack.reserve(9 * levels);
            UInt32* esp = (UInt32*)info->ContextRecord->Esp;
            try {
               UInt32 i = 0;
               char   s[9];
               do {
                  UInt32 p = esp[i];
                  sprintf(s, "%08X", p);
                  if (_AddressIsProbablyVanillaCode(p)) {
                     if (!stack.empty())
                        _MESSAGE("      %s", stack.c_str());
                     _MESSAGE("   %s", s);
                     stack.clear();
                     continue;
                  }
                  stack += s;
                  stack += ' ';
               } while (++i < levels);
               if (!stack.empty())
                  _MESSAGE("      %s", stack.c_str());
            } catch (...) {
               _MESSAGE("   FAILED TO PRINT.");
            }
         }
         {  // Module debug.
            _MESSAGE("\n");
            constexpr int LOAD_COUNT = 140;
            HANDLE  processHandle = GetCurrentProcess();
            HMODULE modules[LOAD_COUNT];
            DWORD   bytesNeeded;
            bool    success = EnumProcessModules(processHandle, modules, sizeof(modules), &bytesNeeded);
            bool    overflow = success && (bytesNeeded > (LOAD_COUNT * sizeof(HMODULE)));
            if (success) {
               UInt32 count = (std::min)(bytesNeeded / sizeof(HMODULE), (UInt32)LOAD_COUNT);
               {  // Identify faulting module.
                  MODULEINFO moduleData;
                  bool       found = false;
                  for (UInt32 i = 0; i < count; i++) {
                     if (GetModuleInformation(processHandle, modules[i], &moduleData, sizeof(moduleData))) {
                        UInt32 base = (UInt32)moduleData.lpBaseOfDll;
                        UInt32 end  = base + moduleData.SizeOfImage;
                        if (eip >= base && eip < end) {
                           TCHAR szModName[MAX_PATH];
                           if (GetModuleFileNameEx(processHandle, modules[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                              found = true;
                              _MESSAGE("GAME CRASHED AT INSTRUCTION Base+0x%08X IN MODULE: %s", (eip - base), szModName);
                              _MESSAGE("Please note that this does not automatically mean that that module is responsible. \n"
                                       "It may have been supplied bad data or program state as the result of an issue in \n"
                                       "the base game or a different DLL.");
                              break;
                           }
                        }
                     }
                  }
                  if (!found) {
                     _MESSAGE("UNABLE TO IDENTIFY MODULE CONTAINING THE CRASH ADDRESS.");
                     _MESSAGE("This can occur if the crashing instruction is located in the vanilla address space, \n"
                              "but it can also occur if there are too many DLLs for us to list, and if the crash \n"
                              "occurred in one of their address spaces. Please note that even if the crash occurred \n"
                              "in vanilla code, that does not necessarily mean that it is a vanilla problem. The \n"
                              "vanilla code may have been supplied bad data or program state as the result of an \n"
                              "issue in a loaded DLL.");
                  }
                  _MESSAGE("\n");
               }
               _MESSAGE("LISTING MODULE BASES (UNORDERED)...");
               try {
                  for (UInt32 i = 0; i < count; i++) {
                     TCHAR szModName[MAX_PATH];
                     if (GetModuleFileNameEx(processHandle, modules[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                        _MESSAGE(TEXT(" - 0x%08X: %s"), modules[i], szModName);
                     }
                  }
                  if (overflow)
                     _MESSAGE("TOO MANY MODULES TO LIST!");
                  _MESSAGE("END OF LIST.");
               } catch (...) {
                  _MESSAGE("   FAILED TO PRINT.");
               }
               _MESSAGE("\nALL DATA PRINTED.");
            } else {
               _MESSAGE("UNABLE TO EXAMINE LOADED DLLs.");
            }
         }
      };
      LONG WINAPI Filter(EXCEPTION_POINTERS* info) {
         static bool caught = false;
         bool ignored = false;
         //
         if (!caught) {
            caught = true;
            try {
               Log(info);
            } catch (...) {};
         } else
            ignored = true;
         if (s_originalFilter)
            s_originalFilter(info); // don't return
         return !ignored ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
      };
      LPTOP_LEVEL_EXCEPTION_FILTER WINAPI FakeSetUnhandledExceptionFilter(__in LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) {
         s_originalFilter = lpTopLevelExceptionFilter;
         return nullptr;
      }
      //
      void Apply() {
         s_originalFilter = SetUnhandledExceptionFilter(&Filter);
         //
         // Prevent Oblivion from disabling the filter:
         //
         SafeWrite32(0x00A281B4, (UInt32)&FakeSetUnhandledExceptionFilter);
      };
   };
};