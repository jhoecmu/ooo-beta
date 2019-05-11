#ifndef RMAP_H
#define RMAP_H
/*********************************************************************
Copyright 2019 James C. Hoe

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/

#include "sim.h"
#include "arch.h"
#include "uarch.h"

#include "regfile.h"

#if (UARCH_ROB_RENAME)
#define MAX_RMAP_READ (UARCH_DECODE_WIDTH*(2))
#else
#define MAX_RMAP_READ (UARCH_DECODE_WIDTH*(2+1))  // new to read tdOld
#endif
#define MAX_RMAP_WRITE (UARCH_DECODE_WIDTH)
#define MAX_RMAP_CHECKPOINT (1)
#if (UARCH_ROB_RENAME)
#define MAX_RMAP_UNMAP (UARCH_RETIRE_WIDTH)
#endif

class RMap {
 public:
  //
  // external interfaces
  //
  RenameTag q2GetMap(LogicalRegName lreg);  // lookup logical to physical mapping

  void a2SetMap(LogicalRegName lreg, RenameTag preg);

  void a2CheckPoint(ULONG which);

  void a6Rewind(ULONG which);

#if (UARCH_ROB_RENAME)
  void a7Unmap(LogicalRegName lreg, RenameTag old);
#endif

  void rReset();

  void simTick();

  // Constructor
  RMap();

 private:
  RenameTag mStack[UARCH_SPECULATE_DEPTH][ARCH_NUM_LOGICAL_REG];
  RenameTag mArray[ARCH_NUM_LOGICAL_REG];
  ULONG dNumRead;
  ULONG dNumWrite;
  ULONG dNumUnmap;
  ULONG dNumCheckpoint;
};


typedef struct {
  ULONG howmany;
  Operation op[UARCH_DECODE_WIDTH];
#if (!UARCH_ROB_RENAME)
  RenameTag tdOld[UARCH_DECODE_WIDTH];
#endif
} RMapBundle;

class RMapSS : public RMap {
 public:
  RMapBundle q2GetMapSS(ULONG howmany,  
			Instruction inst[UARCH_DECODE_WIDTH],  
			RenameTag free[UARCH_DECODE_WIDTH]); // lookup logical to physical mapping
  
#if (!UARCH_ROB_RENAME)
  void a0UnmapSS(ULONG howmany,  
		 LogicalRegName rd[UARCH_DECODE_WIDTH], 
		 RenameTag tdOld[UARCH_DECODE_WIDTH]); 
#endif

  void a2SetMapSS(ULONG howmany,  
		  Instruction inst[UARCH_DECODE_WIDTH], 
		  RenameTag free[UARCH_DECODE_WIDTH]); // set new logical to physical mapping
  
  // Constructor
  RMapSS();
  
 private:
  //  RMap rmap;
};

#endif
