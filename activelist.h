#ifndef ACTIVELIST_H
#define ACTIVELIST_H
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
#include "magic.h"

#include "regfile.h"
#include "rmap.h"
#include "instq.h"

#define MAX_ACTIVELIST_READPC (1)
#define MAX_ACTIVELIST_READOLD (1)
#define MAX_ACTIVELIST_READFREE (1)
#define MAX_ACTIVELIST_READSTATUS (1)
#define MAX_ACTIVELIST_ACCEPT (1) 
#define MAX_ACTIVELIST_COMPLETE (UARCH_EXECUTE_WIDTH)
#define MAX_ACTIVELIST_EXCEPT (UARCH_EXECUTE_WIDTH)
#define MAX_ACTIVELIST_RETIRE (1)

typedef struct {
  ULONG howmany;
  RenameTag free[UARCH_DECODE_WIDTH];
  ULONG atag[UARCH_DECODE_WIDTH];
} FreeRegBundle;

#if (!UARCH_ROB_RENAME)
typedef struct {
  ULONG howmany;
  RenameTag tdOld[UARCH_DECODE_WIDTH];
  LogicalRegName rd[UARCH_DECODE_WIDTH];
} UnmapBundle;
#endif

typedef struct {
  ULONG pcLike;
  bool completed;
  bool exception;
  LogicalRegName rd;
#if (UARCH_DRIS_CHECKER)
  // DRIS combines ROB, RS, and rename functionalities into one
  // centralized data structure. This is based on Metaflow Lightning
  // DRIS. This is only used as a checker in this project.
  LogicalRegName drisRs1, drisRs2;
  RenameTag drisTd, drisTs1, drisTs2;
  bool drisIssued, drisTs1Rdy, drisTs2Rdy;
#endif
#if (!UARCH_ROB_RENAME)
  RenameTag tdNew; // this is the "freelist"
  RenameTag tdOld; // need this to unwind on exception
#endif
  Cookie cookie;
} ActiveListEntry;

typedef struct {
  ULONG howmany;
  RenameTag td[UARCH_RETIRE_WIDTH];
#if (UARCH_ROB_RENAME)
  LogicalRegName rd[UARCH_RETIRE_WIDTH];
  DataValue val[UARCH_RETIRE_WIDTH];
  Cookie cookie[UARCH_RETIRE_WIDTH];
#endif
} RetireBundle;

class ActiveList {
 public:
#if (!UARCH_ROB_RENAME)
  UnmapBundle q0Unmap();
#endif
  bool q0HandleException();
  ULONG q0GetPC(ULONG activeListIdx);
  ULONG q0GetExceptionPC();
  
  FreeRegBundle q2GetFreeReg();

#if (!UARCH_ROB_RENAME)
  void a0Unmap(ULONG howmany);
#endif

  RetireBundle q7toRetire();

  void a2Accept(ULONG howmany, 
		Instruction inst[UARCH_DECODE_WIDTH], ULONG pcLike[UARCH_DECODE_WIDTH],
#if (!UARCH_ROB_RENAME)
		RenameTag tdOld[UARCH_DECODE_WIDTH],
#endif
#if (UARCH_DRIS_CHECKER)
		RMapBundle renameBndl,
#endif
		Cookie cookie[UARCH_DECODE_WIDTH]);
  void a2CheckPoint(ULONG which);
  
  void a6Complete(ULONG activeListIdx);
  void a6Exception(ULONG activeListIdx);
#if (UARCH_ROB_RENAME)
  void a6Rewind(ULONG activeListIdx);
#else
  void a6Rewind(ULONG which);
#endif

  void a7Retire(RetireBundle bundle);
  
#if (UARCH_DRIS_CHECKER)
  void d4CheckIssue(InstQEntry issue);
#endif
  
  void rReset();

  void simTick();

  // Constructor
  ActiveList();

 private:
  ActiveListEntry mArray[UARCH_OOO_DEGREE];
#if (!UARCH_ROB_RENAME)
  ULONG mEnqPtrStack[UARCH_SPECULATE_DEPTH];
#endif
  ULONG mEnqPtr;
  ULONG mDeqPtr;

  ULONG dNumReadPC;
  ULONG dNumReadOld;
  ULONG dNumReadFree;
  ULONG dNumReadStatus;
  ULONG dNumAccept;
  ULONG dNumComplete;
  ULONG dNumExcept;
  ULONG dNumRetire;

  ULONG sizeActiveList();
  bool isOlder(ULONG young, ULONG old);
  void printState();
};

#endif
