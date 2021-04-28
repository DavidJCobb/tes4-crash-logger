#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"

#include "Patches/CrashLog.h"

IDebugLog    gLog("Data\\OBSE\\Plugins\\CobbCrashLogger.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

extern "C" {
   bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info) {
      // fill out the info structure
      info->infoVersion = PluginInfo::kInfoVersion;
      info->name        = "CobbCrashLogger";
      info->version     = 0x02000000; // major, minor, patch, build

      {  // log our version number -- be helpful!
         auto v = info->version;
         UInt8 major = v >> 0x18;
         UInt8 minor = (v >> 0x10) & 0xFF;
         UInt8 patch = (v >> 0x08) & 0xFF;
         UInt8 build = v & 0xFF;
         _MESSAGE("Cobb Crash Logger Version %d.%d.%d, build %d.", major, minor, patch, build);
         _MESSAGE("If this file is empty, then your game didn't crash! Probably!\n");
      }

      // version checks
      if(!obse->isEditor) {
         if(obse->obseVersion < 21) {
            _ERROR("OBSE version too old (got %08X; expected at least %08X).", obse->obseVersion, 21);
            return false;
         }
         #if OBLIVION
            if(obse->oblivionVersion != OBLIVION_VERSION_1_2_416) {
               _ERROR("incorrect Oblivion version (got %08X; need %08X).", obse->oblivionVersion, OBLIVION_VERSION_1_2_416);
               return false;
            }
         #endif
      } else {
         // no version checks needed for editor
      }
      // version checks pass
      return true;
   }

   bool OBSEPlugin_Load(const OBSEInterface* obse) {
      g_pluginHandle = obse->GetPluginHandle();
      //
      if (!obse->isEditor)
         CobbPatches::CrashLog::Apply();
      return true;
   }
};
