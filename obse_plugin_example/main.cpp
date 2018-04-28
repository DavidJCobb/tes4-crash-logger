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
	   info->version     = 0x00000000; // major, minor, patch, build

	   // version checks
	   if(!obse->isEditor) {
		   if(obse->obseVersion < OBSE_VERSION_INTEGER && obse->obseVersion < 21) {
            _ERROR("OBSE version too old (got %08X; expected at least %08X).", obse->obseVersion, OBSE_VERSION_INTEGER);
			   return false;
		   }
         #if OBLIVION
		      if(obse->oblivionVersion != OBLIVION_VERSION) {
               _ERROR("incorrect Oblivion version (got %08X; need %08X).", obse->oblivionVersion, OBLIVION_VERSION);
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
