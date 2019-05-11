#define ACTIVELIST_CPP
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

#include "print.h"

#include "activelist.h"
#include "checkpoint.h"
#include "regfile.h"
#include "rmap.h"

#define MARRAY(j) (mArray[(j)%UARCH_OOO_DEGREE])
 
ULONG ActiveList::sizeActiveList() {
  ULONG size;
  ULONG enqColor=mEnqPtr/UARCH_OOO_DEGREE;
  ULONG enqIdx=mEnqPtr%UARCH_OOO_DEGREE;
  ULONG deqColor=mDeqPtr/UARCH_OOO_DEGREE;
  ULONG deqIdx=mDeqPtr%UARCH_OOO_DEGREE;

  ASSERT(enqColor<=1);
  ASSERT(deqColor<=1);
  
  if (enqColor==deqColor) {
    size=enqIdx-deqIdx;
  } else {
    size=(enqIdx+UARCH_OOO_DEGREE)-deqIdx;
  }
  return size;
}

bool ActiveList::isOlder(ULONG young, ULONG old) {
  ULONG size;
  ULONG oldColor=old/UARCH_OOO_DEGREE;
  ULONG oldIdx=old%UARCH_OOO_DEGREE;
  ULONG youngColor=young/UARCH_OOO_DEGREE;
  ULONG youngIdx=young%UARCH_OOO_DEGREE;

  ASSERT(oldColor<=1);
  ASSERT(youngColor<=1);
  
  if (oldColor==youngColor) {
    return (youngIdx>oldIdx);
  } else {
    return (youngIdx<=oldIdx);
  }
  return size;
}

ULONG ActiveList::q0GetPC(ULONG activeListIdx) {
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumReadPC++)<MAX_ACTIVELIST_READPC), "exceeding number of ActiveList PC read port limit\n");

#if (UARCH_ROB_RENAME)
  ASSERT(activeListIdx<(2*UARCH_OOO_DEGREE));
#else
  ASSERT(activeListIdx<UARCH_OOO_DEGREE);
#endif
  ASSERT(MARRAY(activeListIdx).pcLike==
	 MARRAY(activeListIdx).cookie.serial);

  return MARRAY(activeListIdx).pcLike;
}

ULONG ActiveList::q0GetExceptionPC() {
  USAGEWARN((!simTock), "query after TOCK");

  ASSERT(MARRAY(mDeqPtr).completed);
  ASSERT(MARRAY(mDeqPtr).exception);

  return q0GetPC(mDeqPtr%UARCH_OOO_DEGREE);
}

#if (!UARCH_ROB_RENAME)
UnmapBundle ActiveList::q0Unmap() {
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumReadOld++)<MAX_ACTIVELIST_READOLD), "exceeding number of ActiveList oldmap read (N) port limit\n");

  ASSERT(sizeActiveList()<=UARCH_OOO_DEGREE);

  UnmapBundle bundle;
  ULONG howmany=MIN(sizeActiveList(), UARCH_DECODE_WIDTH);

  bundle.howmany=howmany;
    
  for(ULONG i=0, j=mEnqPtr;i<howmany;i++) { 
    j--;
    j%=(2*UARCH_OOO_DEGREE);
    
    bundle.tdOld[i]=MARRAY(j).tdOld;
    bundle.rd[i]=MARRAY(j).rd;
  }

  return bundle;
}
#endif

FreeRegBundle ActiveList::q2GetFreeReg() {
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumReadFree++)<MAX_ACTIVELIST_READFREE), "exceeding number of ActiveList free reg read (N) port limit\n");

  ASSERT(sizeActiveList()<=UARCH_OOO_DEGREE);

  FreeRegBundle bundle;
  ULONG remaining=UARCH_OOO_DEGREE-sizeActiveList();
  ULONG howmany=(remaining>=UARCH_DECODE_WIDTH)?UARCH_DECODE_WIDTH:remaining;

  bundle.howmany=howmany;
    
  for(ULONG i=0, j=mEnqPtr;i<howmany;i++) { 
#if (UARCH_ROB_RENAME)
    bundle.free[i].mapped=true;
    bundle.free[i].idx=(j%(1*UARCH_OOO_DEGREE));

    bundle.atag[i]=(j%(2*UARCH_OOO_DEGREE));
#else
    bundle.free[i]=MARRAY(j).tdNew;

    bundle.atag[i]=(j%UARCH_OOO_DEGREE);
#endif
    j++;
    j%=(2*UARCH_OOO_DEGREE);
  }

  return bundle;
}

RetireBundle  ActiveList::q7toRetire() {
  USAGEWARN((!simTock), "query after TOCK");
  printState();
  
  RetireBundle bundle;
  ULONG howmany=0;
  
#if (UARCH_ROB_RENAME)
  FOR_RETIRE_WIDTH_i { bundle.td[i].idx=0; }
#endif

  for(ULONG i=0, j=mDeqPtr; i<UARCH_RETIRE_WIDTH; i++) {
    if (j==mEnqPtr) { 
      break;
    }
    if ((!MARRAY(j).completed)||
	MARRAY(j).exception) {
      break;
    }

#if (UARCH_ROB_RENAME)
    bundle.rd[i]=MARRAY(j).rd;
    {
      RenameTag temp={.mapped=true, .idx=(j%(1*UARCH_OOO_DEGREE))};
      bundle.td[i]=MARRAY(j).rd?temp:ZeroRegTag;
    }
    bundle.cookie[i]=MARRAY(j).cookie;
#else
    bundle.td[i]=MARRAY(j).tdOld;
#endif

    howmany++;
    j++;
    j%=(2*UARCH_OOO_DEGREE);
  }

  
  bundle.howmany=howmany;

  ASSERT(howmany<=UARCH_RETIRE_WIDTH);
  ASSERT(sizeActiveList()>=howmany);

  return bundle;
}

bool ActiveList::q0HandleException() {
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumReadStatus++)<MAX_ACTIVELIST_READSTATUS), "exceeding number of ActiveList oldest status read port limit\n");

  if (mDeqPtr!=mEnqPtr) {
    if (MARRAY(mDeqPtr).completed) {
      if (MARRAY(mDeqPtr).exception) {
	return true;
      }
    }
  }
  return false;
}

#if (!UARCH_ROB_RENAME)
void ActiveList::a0Unmap(ULONG howmany) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(sizeActiveList()<=UARCH_OOO_DEGREE);
  ASSERT(sizeActiveList()>=howmany);

  mEnqPtr-=howmany;
  mEnqPtr%=(2*UARCH_OOO_DEGREE);

  ASSERT(sizeActiveList()<=UARCH_OOO_DEGREE);
}
#endif

void ActiveList::a2Accept(ULONG howmany, Instruction inst[UARCH_DECODE_WIDTH], ULONG pcLike[UARCH_DECODE_WIDTH], 
#if (!UARCH_ROB_RENAME)
    RenameTag tdOld[UARCH_DECODE_WIDTH], 
#endif
#if (UARCH_DRIS_CHECKER)
    RMapBundle renameBndl,
#endif
    Cookie cookie[UARCH_DECODE_WIDTH]) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumAccept++)<MAX_ACTIVELIST_ACCEPT), "exceeding number of ActiveList accept write (N) port limit\n");

  ASSERT(howmany<=(UARCH_OOO_DEGREE-sizeActiveList()));

  for(ULONG i=0, j=mEnqPtr;i<howmany;i++) {
    MARRAY(j).completed=false;
    MARRAY(j).exception=false;
    MARRAY(j).pcLike=pcLike[i];
    MARRAY(j).rd=inst[i].rd;
#if (!UARCH_ROB_RENAME)
    MARRAY(j).tdOld=tdOld[i];
#endif
    MARRAY(j).cookie=cookie[i];

    ASSERT(MARRAY(j).pcLike==
	   MARRAY(j).cookie.serial);

#if (UARCH_DRIS_CHECKER)
    MARRAY(j).drisRs1=inst[i].rs1;
    MARRAY(j).drisRs2=inst[i].rs2;

    if (inst[i].rd) {
      MARRAY(j).drisTd.mapped=true;
      MARRAY(j).drisTd.idx=j;
    } else {
      MARRAY(j).drisTd=ZeroRegTag;
    }

    {
      MARRAY(j).drisTs1Rdy=true;
      MARRAY(j).drisTs1.mapped=false;
      MARRAY(j).drisTs1.idx=inst[i].rs1;
      MARRAY(j).drisTs2Rdy=true;
      MARRAY(j).drisTs2.mapped=false;
      MARRAY(j).drisTs2.idx=inst[i].rs2;
      
      if (inst[i].rs1!=R0) {
	for(ULONG k=j; k!=mDeqPtr; ) {
	  k--;
	  k%=2*UARCH_OOO_DEGREE;
	  if (inst[i].rs1==MARRAY(k).rd) {
	    MARRAY(j).drisTs1=MARRAY(k).drisTd;
	    MARRAY(j).drisTs1Rdy=MARRAY(k).drisIssued;  // ready when issued
	    // since alu 1-cycle
	    // with forwarding
	    break;
	  }
	}
      }


      if (inst[i].rs2!=R0) {
	for(ULONG k=j; k!=mDeqPtr; ) {
	  k--;
	  k%=2*UARCH_OOO_DEGREE;
	  if (inst[i].rs2==MARRAY(k).rd) {
	    MARRAY(j).drisTs2=MARRAY(k).drisTd;
	    MARRAY(j).drisTs2Rdy=MARRAY(k).drisIssued;
	    break;
	  }
	}
      }
    }
    ASSERT(drisTagIdxEqual(MARRAY(j).drisTd,renameBndl.op[i].td));
    ASSERT(drisTagIdxEqual(MARRAY(j).drisTs1,renameBndl.op[i].ts1));
    ASSERT(drisTagIdxEqual(MARRAY(j).drisTs2,renameBndl.op[i].ts2));

    MARRAY(j).drisIssued=false;

#endif

    j++;
    j%=(2*UARCH_OOO_DEGREE);
  }

  mEnqPtr+=howmany;
  mEnqPtr%=(2*UARCH_OOO_DEGREE);
}

void ActiveList::a6Complete(ULONG activeListIdx) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumComplete++)<MAX_ACTIVELIST_COMPLETE), "exceeding number of ActiveList complete set port limit\n");

  ASSERT(!MARRAY(activeListIdx).completed);

  MARRAY(activeListIdx).completed=true;

#if (UARCH_DRIS_CHECKER)
  ASSERT(MARRAY(activeListIdx).drisIssued);
#endif
}

void ActiveList::a6Exception(ULONG activeListIdx) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumExcept++)<MAX_ACTIVELIST_EXCEPT), "exceeding number of ActiveList except set port limit\n");

  ASSERT(!MARRAY(activeListIdx).exception);

  MARRAY(activeListIdx).exception=true;
}

void ActiveList::a7Retire(RetireBundle bundle) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumRetire++)<MAX_ACTIVELIST_RETIRE), "exceeding number of ActiveList retire status read (N) port limit\n");
  ASSERT(bundle.howmany<=UARCH_RETIRE_WIDTH);
  ASSERT(sizeActiveList()>=bundle.howmany);

  for(ULONG i=0, j=mDeqPtr; i<bundle.howmany; i++) {
    ASSERT(!(j==mEnqPtr));
    ASSERT(!((!MARRAY(j).completed)||
	     MARRAY(j).exception==true));

#if (!UARCH_ROB_RENAME)
    MARRAY(j).tdNew=bundle.td[i];
#endif

#if (DEBUG_LEVEL>=DEBUG_FULL)
    prettyPrint(RSTAGE, MARRAY(j).cookie.op, MARRAY(j).cookie);
#endif

    j++;
    j%=(2*UARCH_OOO_DEGREE);
  }

  mDeqPtr+=bundle.howmany;
  mDeqPtr%=(2*UARCH_OOO_DEGREE);
}


#if (!UARCH_ROB_RENAME)
void ActiveList::a2CheckPoint(ULONG which) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(which<UARCH_SPECULATE_DEPTH);

  mEnqPtrStack[which]=mEnqPtr;

  return;
}
#endif
 
#if (UARCH_ROB_RENAME)
void ActiveList::a6Rewind(ULONG activeListIdx) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(activeListIdx<(2*UARCH_OOO_DEGREE));

  activeListIdx++;
  activeListIdx%=(2*UARCH_OOO_DEGREE);

  mEnqPtr=activeListIdx;

  return;
}
#else
void ActiveList::a6Rewind(ULONG which) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(which<UARCH_SPECULATE_DEPTH);

  mEnqPtr=mEnqPtrStack[which];

  return;
}
#endif

#if (UARCH_DRIS_CHECKER)
void ActiveList::d4CheckIssue(InstQEntry issue) {
  ASSERT(issue.valid);
  ASSERT(isOlder(mEnqPtr,issue.atag));
  ASSERT(!isOlder(mDeqPtr,issue.atag));

  if (isOlder(mDeqPtr,MARRAY(issue.atag).drisTs1.idx)) {
    RenameTag temp={.mapped=false, .idx=MARRAY(issue.atag).drisRs1};
    ASSERT(tagEqual(issue.op.ts1,temp));
  } else {
    ASSERT(drisTagIdxEqual(issue.op.ts1,MARRAY(issue.atag).drisTs1));
  }
  if (isOlder(mDeqPtr,MARRAY(issue.atag).drisTs2.idx)) {
    RenameTag temp={.mapped=false, .idx=MARRAY(issue.atag).drisRs2};
    ASSERT(tagEqual(issue.op.ts2,temp));
  } else {
    ASSERT(drisTagIdxEqual(issue.op.ts2,MARRAY(issue.atag).drisTs2));
  }
  ASSERT(MARRAY(issue.atag).drisTs1Rdy);
  ASSERT(MARRAY(issue.atag).drisTs2Rdy);

  for(ULONG k=mEnqPtr; k!=mDeqPtr; ) {
    k--;
    k%=2*UARCH_OOO_DEGREE;
    if ((MARRAY(issue.atag).rd!=R0) &&
	tagEqual(MARRAY(issue.atag).drisTd,
		 MARRAY(k).drisTs1)) {
      ASSERT(!MARRAY(k).drisTs1Rdy);
      MARRAY(k).drisTs1Rdy=true;
    }
  }
  for(ULONG k=mEnqPtr; k!=mDeqPtr; ) {
    k--;
    k%=2*UARCH_OOO_DEGREE;
    if ((MARRAY(issue.atag).rd!=R0) &&
	tagEqual(MARRAY(issue.atag).drisTd,
		 MARRAY(k).drisTs2)) {
      ASSERT(!MARRAY(k).drisTs2Rdy);
      MARRAY(k).drisTs2Rdy=true;
    }
  }

  ASSERT(!MARRAY(issue.atag).drisIssued);
  MARRAY(issue.atag).drisIssued=true;
}
#endif
 
void ActiveList::rReset() {
  simTick();

#if (!UARCH_ROB_RENAME)
  for(ULONG i=0; i<UARCH_OOO_DEGREE; i++) {
    RenameTag temp={.idx=ARCH_NUM_LOGICAL_REG+i};
    MARRAY(i).tdNew=temp;
  }
#endif

  mEnqPtr=0;
  mDeqPtr=0;
}

void ActiveList::simTick() {
  dNumReadPC=0;
  dNumReadOld=0;
  dNumReadFree=0;
  dNumReadStatus=0;
  dNumAccept=0;
  dNumComplete=0;
  dNumExcept=0;
  dNumRetire=0;
}

void ActiveList::printState() {
#if (DEBUG_LEVEL>=DEBUG_VERBOSE)
  {
    bool printing=false;

    for(ULONG i=0, j=mDeqPtr; i<UARCH_OOO_DEGREE; i++) {
      if (j==mEnqPtr) { 
	break;
      }
      if (!printing) {
	cout << "------------------------------------------------------------------------------------\n";
	printing=true;
      }
      prettyPrint(":InActLST:", MARRAY(j).cookie.op, MARRAY(j).cookie, "\t\t", MARRAY(j).completed?" completed":"");

      j++;
      j%=(2*UARCH_OOO_DEGREE);
    }
    if (printing) {
      cout << "------------------------------------------------------------------------------------\n";
    }
  }
#endif
}

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////

ActiveList::ActiveList() {
  cout << "MAX_ACTIVELIST_READPC=" << MAX_ACTIVELIST_READPC << "\n";
  cout << "MAX_ACTIVELIST_READOLD=" << MAX_ACTIVELIST_READOLD << "\n";
  cout << "MAX_ACTIVELIST_READFREE=" << MAX_ACTIVELIST_READFREE << "\n";
  cout << "MAX_ACTIVELIST_READSTATUS=" << MAX_ACTIVELIST_READSTATUS << "\n";
  cout << "MAX_ACTIVELIST_ACCEPT=" << MAX_ACTIVELIST_ACCEPT << "\n";
  cout << "MAX_ACTIVELIST_COMPLETE=" << MAX_ACTIVELIST_COMPLETE << "\n";
  cout << "MAX_ACTIVELIST_EXCEPT=" << MAX_ACTIVELIST_EXCEPT << "\n";
  cout << "MAX_ACTIVELIST_RETIRE=" << MAX_ACTIVELIST_RETIRE << "\n";

  rReset();
}


