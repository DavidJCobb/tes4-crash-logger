#pragma once

//
// TODO: use this to log a better stack trace
//

struct Label {
   enum Type : UInt8 {
      kType_Subroutine = 0,
      kType_Address    = 1,
      kType_VTBL       = 2,
      kType_Constant   = 3,
   };
   //
   const char* name;
   UInt32 start;
   UInt16 size;
   Type   type = kType_Subroutine;
   //
   Label(UInt32 b, UInt16 c, const char* a) : name(a), start(b), size(c) {};
   Label(UInt32 b, UInt16 c, const char* a, Type d) : name(a), start(b), size(c), type(d) {};
};
extern const Label  g_labels[]; // std::extent doesn't work from outside of the CPP file
extern const UInt32 g_labelCount;