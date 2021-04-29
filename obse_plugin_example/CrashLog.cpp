#include "CrashLog.h"
#include "definitions.h"
#include "StackWalk.h"
#include "obse_common/SafeWrite.h"

constexpr UInt32 ce_printStackCount = 50;

namespace CobbPatches {
   namespace Subroutines {
      const Label* GetLabel(UInt32 addr) {
         for (UInt32 i = 0; i < g_labelCount; i++) {
            auto&  entry = g_labels[i];
            if (addr >= entry.start && addr <= (entry.start + entry.size))
               return &entry;
         }
         return nullptr;
      }
   }
   namespace CrashLog {
      static LPTOP_LEVEL_EXCEPTION_FILTER s_originalFilter = nullptr;

      struct UserContext {
          UInt32 eip;
          UInt32 moduleBase;
          CHAR* name;
      };

      BOOL CALLBACK EumerateModulesCallback(PCSTR name, ULONG moduleBase, ULONG moduleSize,  PVOID context) {
          UserContext* info = (UserContext*)context;
          if (info->eip >= (UInt32)moduleBase && info->eip <= (UInt32)moduleBase + (UInt32)moduleSize) {
              //_MESSAGE("%0X %0X  %0X", (UInt32)moduleBase, info->eip, (UInt32)moduleBase + (UInt32) moduleSize);
              info->moduleBase = moduleBase;
              UInt32 len = strlen(name);
              info->name = (char*)calloc(1, len);
              strcpy_s(info->name, len, name);
          }
          _MESSAGE(" - 0x%08X - 0x%08X: %s", (UInt32)moduleBase, (UInt32)moduleBase + (UInt32)moduleSize, name);
          return TRUE;
      }

      BOOL CALLBACK EumerateModulesCallbackSym(PCSTR name, ULONG moduleBase, PVOID context) {
          UserContext* info = (UserContext*)context;
          IMAGEHLP_MODULE modu;
          SymGetModuleInfo(GetCurrentProcess(), moduleBase, &modu );
          if (info->eip >= (UInt32) moduleBase && info->eip <= (UInt32)moduleBase + modu.ImageSize) {
  //            _MESSAGE("%0X %0X  %0X", (UInt32)moduleBase, info->eip, (UInt32)moduleBase + moduleSize);
              info->moduleBase = moduleBase;
              UInt32 len = strlen(name);
              info->name = (char*)calloc(1, len);
              strcpy_s(info->name, len,  name);
          }
          _MESSAGE(" - 0x%08X - 0x%08X: %s", (UInt32)moduleBase, (UInt32)moduleBase + modu.ImageSize, name);
          return TRUE;
      }

      void Log(EXCEPTION_POINTERS* info) {
          HANDLE  processHandle = GetCurrentProcess();
          HANDLE  threadHandle = GetCurrentThread();
          _MESSAGE("Exception %08X caught!", info->ExceptionRecord->ExceptionCode);
          _MESSAGE("\nCalltrace: ");
          StackWalk(info, processHandle, threadHandle);

          UInt32 eip = info->ContextRecord->Eip;
         _MESSAGE("\nInstruction pointer (EIP): %08X", eip);
         _MESSAGE("\nREG | VALUE");
         _MESSAGE("%s | %08X", "eax", info->ContextRecord->Eax);
         _MESSAGE("%s | %08X", "ebx", info->ContextRecord->Ebx);
         _MESSAGE("%s | %08X", "ecx", info->ContextRecord->Ecx);
         _MESSAGE("%s | %08X", "edx", info->ContextRecord->Edx);
         _MESSAGE("%s | %08X", "edi", info->ContextRecord->Edi);
         _MESSAGE("%s | %08X", "esi", info->ContextRecord->Esi);
         _MESSAGE("%s | %08X", "ebp", info->ContextRecord->Ebp);
         UInt32* esp = (UInt32*)info->ContextRecord->Esp;
         for (unsigned int i = 0; i < ce_printStackCount; i+=4) {
             _MESSAGE("0x%08X | 0x%08X | 0x%08X | 0x%08X", esp[i], esp[i+1], esp[i+2], esp[i+3]);
         }
         _MESSAGE("\nLISTING MODULE BASES ...");
         UserContext infoUser = { eip, 0 , NULL};
         EnumerateLoadedModules(processHandle, EumerateModulesCallback, &infoUser);
//        SymEnumerateModules(processHandle, EumerateModulesCallbackSym, &infoUser);
         if (infoUser.moduleBase) {
             _MESSAGE("\nGAME CRASHED AT INSTRUCTION Base+0x%08X IN MODULE: %s", (infoUser.eip - infoUser.moduleBase), infoUser.name);
             _MESSAGE("Please note that this does not automatically mean that that module is responsible. \n"
                 "It may have been supplied bad data or program state as the result of an issue in \n"
                 "the base game or a different DLL.");
         }else{
             _MESSAGE("\nUNABLE TO IDENTIFY MODULE CONTAINING THE CRASH ADDRESS.");
             _MESSAGE("This can occur if the crashing instruction is located in the vanilla address space, \n"
                 "but it can also occur if there are too many DLLs for us to list, and if the crash \n"
                 "occurred in one of their address spaces. Please note that even if the crash occurred \n"
                 "in vanilla code, that does not necessarily mean that it is a vanilla problem. The \n"
                 "vanilla code may have been supplied bad data or program state as the result of an \n"
                 "issue in a loaded DLL.");
         }

         SymCleanup(processHandle);
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
      void Apply(bool IsNewVegas) {
         s_originalFilter = SetUnhandledExceptionFilter(&Filter);
         //
         // Prevent disabling the filter:
         //
         if(IsNewVegas) SafeWrite32(0x00FDF180, (UInt32)&FakeSetUnhandledExceptionFilter);
         else SafeWrite32(0x00A281B4, (UInt32)&FakeSetUnhandledExceptionFilter);
         
      };
   };
};
