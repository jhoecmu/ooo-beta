#ifndef INSTQ_H
#define INSTQ_H
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

#define MAX_INSTQ_READY (1)
#define MAX_INSTQ_INSERT (UARCH_DECODE_WIDTH)
#define MAX_INSTQ_ISSUE (1)
#define MAX_INSTQ_RELEASE (UARCH_EXECUTE_WIDTH)
#define MAX_INSTQ_RETIRE (UARCH_RETIRE_WIDTH)
#define MAX_INSTQ_SQUASH (1)
#define MAX_INSTQ_CLEAR (1)

#define INSTQ_MSCAN_RANDOM (0)
#define INSTQ_MSCAN_RROBIN (1)
#define MSCANSTART (((mScan*INSTQ_MSCAN_RROBIN)+(INSTQ_MSCAN_RANDOM?rand():0))%UARCH_INSTQ_SIZE)

typedef struct {
  ULONG slotIdx;
  bool valid;
  ULONG atag;
  Operation op;
  bool ts1Ready, ts2Ready;
  Cookie cookie;
} InstQEntry; 

static const InstQEntry invalidEntryInstQ={.slotIdx=0, .valid=false}; // empty stub value

class InstQ {
 public:
  //
  // external interfaces
  //
  ULONG q2NumSlots();

  InstQEntry q4Readied(); 

  void a3Insert(ULONG ptag, Operation op, 
		bool ts1Busy, bool ts2Busy, 
		Cookie cookie);


  void a4Issue(ULONG which);
  void a4Release(RenameTag tag, Cookie cookie);

  void a6Squash(SpeculateMask mask, Cookie cookie);
  void a6ClearMask(SpeculateMask mask, Cookie cookie);

#if (UARCH_ROB_RENAME)
  void a7retireTag(RenameTag ptag, RenameTag ltag, Cookie cookie);
#endif
  
  void rReset();
  void simTick();

  // Constructor
  InstQ();

 private:
  ULONG mInUse;
  ULONG mScan;
  //InstQEntry mNoBody;
  InstQEntry mArray[UARCH_INSTQ_SIZE];  // change to dyanmic allocate for instq of different depth

  ULONG dNumReadied;
  ULONG dNumInsert;
  ULONG dNumIssue;
  ULONG dNumRelease;
  ULONG dNumRetire;
  ULONG dNumSquash;
  ULONG dNumClear;

  void printState();
};

#endif
