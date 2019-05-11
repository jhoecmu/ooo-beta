#define RMAP_CPP
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

#include "rmap.h"
#include "checkpoint.h"

////////////////////////////////////////////////////////
//
// interface methods
//
////////////////////////////////////////////////////////

// lookup logical to physical mapping
RenameTag RMap::q2GetMap(LogicalRegName lreg) { 
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumRead++)<MAX_RMAP_READ), "exceeding number of RMap read port limit\n");

  ASSERT(lreg<ARCH_NUM_LOGICAL_REG);

  RenameTag tag;

  if (lreg!=R0) {
#if (UARCH_ROB_RENAME)
    tag.mapped=mArray[lreg].mapped;
    tag.idx=tag.mapped?mArray[lreg].idx:lreg;
#else
    tag=mArray[lreg];
    ASSERT(tag.idx<UARCH_NUM_PHYSICAL_REG);
#endif
  } else {
    tag=ZeroRegTag;
  }


  return tag;
}

// set new logical to physical mapping
void RMap::a2SetMap(LogicalRegName lreg, RenameTag tag) {

  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumWrite++)<MAX_RMAP_WRITE), "exceeding number of RMap write port limit\n");

  ASSERT(lreg<ARCH_NUM_LOGICAL_REG);

  if (lreg!=R0) {
#if (UARCH_ROB_RENAME)
    ASSERT(tag.mapped);
    ASSERT(tag.idx<(1*UARCH_OOO_DEGREE));
#else
    ASSERT(tag.idx<UARCH_NUM_PHYSICAL_REG);
#endif
    
    mArray[lreg]=tag;
  }

  return;
}

void RMap::a2CheckPoint(ULONG which) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumCheckpoint++)<MAX_RMAP_CHECKPOINT), "exceeding number of RMap checkpoint port limit\n");

  ASSERT(which<UARCH_SPECULATE_DEPTH);

  for(ULONG i=0; i<ARCH_NUM_LOGICAL_REG; i++) {
    mStack[which][i]=mArray[i];
  }

  return;
}
 
void RMap::a6Rewind(ULONG which) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(which<UARCH_SPECULATE_DEPTH);

  for(ULONG i=0; i<ARCH_NUM_LOGICAL_REG; i++) {
    mArray[i]=mStack[which][i];
  }

  return;
}

#if (UARCH_ROB_RENAME)
void RMap::a7Unmap(LogicalRegName lreg, RenameTag old) {

  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumUnmap++)<MAX_RMAP_UNMAP), "exceeding number of RMap unmap clear port limit\n");

  ASSERT(lreg<ARCH_NUM_LOGICAL_REG);

  if (lreg!=R0) {
    ASSERT(mArray[lreg].mapped);
    if (tagEqual(mArray[lreg],old)) {
      mArray[lreg].mapped=false;
    }
  }

  // Note: unmap at retirement of an instruction needs to be applied to all checkpoints
  FOR_SPECULATE_DEPTH_i {
    // okay to overwrite all valid or invalid (don't care)
    if (lreg!=R0) {
      if (tagEqual(mStack[i][lreg],old)) {
	mStack[i][lreg].mapped=false;
      }
    }
  }

  return;
}
#endif

void RMap::rReset() { 
  simTick();

  for(ULONG i=0; i<ARCH_NUM_LOGICAL_REG; i++)  {
#if (UARCH_ROB_RENAME)
    mArray[i].mapped=false;
#endif
    mArray[i].idx=i;
  }

  return; 
}                      
void RMap::simTick() { 

  dNumRead=0;
  dNumWrite=0;
  dNumUnmap=0;
  dNumCheckpoint=0;

  return; 
}                      

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
RMap::RMap() {
  cout << "MAX_RMAP_READ=" << MAX_RMAP_READ << "\n";
  cout << "MAX_RMAP_WRITE=" << MAX_RMAP_WRITE << "\n";
  cout << "MAX_RMAP_CHECKPOINT=" << MAX_RMAP_CHECKPOINT << "\n";
#if (UARCH_ROB_RENAME)
  cout << "MAX_RMAP_UNMAP=" << MAX_RMAP_UNMAP << "\n";
#endif

  rReset();
}


// lookup logical to physical mapping upto UARCH_DECODE WIDTH
RMapBundle RMapSS::q2GetMapSS(ULONG howmany,  Instruction inst[UARCH_DECODE_WIDTH],  RenameTag free[UARCH_DECODE_WIDTH]) {
  USAGEWARN((!simTock), "query after TOCK");
  ASSERT(howmany<=UARCH_DECODE_WIDTH);

  RMapBundle renamed;

  renamed.howmany=howmany;

  FOR_DECODE_WIDTH_i {
    // initialize values in case mapping less than full width
    renamed.op[i].ts1=ZeroRegTag;
    renamed.op[i].ts2=ZeroRegTag;
    renamed.op[i].td=ZeroRegTag;  // <== relying on this to not set Busy 
  }

  for(ULONG i=0; i<howmany; i++) {
    // not okay to overrunn; relying on td be 0 on overruns
    renamed.op[i].opcode=inst[i].opcode;

    if (inst[i].rd!=R0) {
      renamed.op[i].td=free[i];
    } else {
      renamed.op[i].td=ZeroRegTag;
    }
  }

  //for(ULONG i=0; i<howmany; i++) {
  FOR_DECODE_WIDTH_i {
    //okay to overrun howmany; overrun ts1/ts2/rd force to 0 anyway
    renamed.op[i].ts1=q2GetMap(inst[i].rs1);
    for(LONG j=i-1; j>=0; j--) {
      if ((inst[i].rs1!=R0) && (inst[i].rs1==inst[j].rd)) {
	renamed.op[i].ts1=renamed.op[j].td;
	break;
      }
    }

    renamed.op[i].ts2=q2GetMap(inst[i].rs2);
    for(LONG j=i-1; j>=0; j--) {
      if ((inst[i].rs2!=R0) && (inst[i].rs2==inst[j].rd)) {
	renamed.op[i].ts2=renamed.op[j].td;
	break;
      }
    }

#if (!UARCH_ROB_RENAME)
    if (inst[i].rd!=R0) {
      // If an instruction has rd!=R0,
      // rd was mapped to a new physical register.
      // At retirement, the new physical register holds commited logical value.
      // The previous rd mapping (looked up here) is returned to the freelist.
      renamed.tdOld[i]=q2GetMap(inst[i].rd);
      for(LONG j=i-1; j>=0; j--) {
	// look for an oldest younger mapping of the same rd this cycle
	if ((inst[i].rd!=R0) && (inst[i].rd==inst[j].rd)) {
	  renamed.tdOld[i]=renamed.op[j].td;
	  break;
	}
      }
    } else {
      // If an instruction has rd==R0
      // R0 is is not mapped a physical register, but
      // it is allocated a physical register for easy of accounting.
      // This register is returned to the "freelist" at retirement; 
      renamed.tdOld[i]=free[i];
    }
#endif
  }

  return renamed;
}

// set new logical to physical mapping
void RMapSS::a2SetMapSS(ULONG howmany,  Instruction inst[UARCH_DECODE_WIDTH],  RenameTag free[UARCH_DECODE_WIDTH]) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(howmany<=UARCH_DECODE_WIDTH);

  for(ULONG i=0; i<howmany; i++) {
    a2SetMap(inst[i].rd, free[i]);
  }
  return;
}

#if (!UARCH_ROB_RENAME)
// set new logical to physical mapping
void RMapSS::a0UnmapSS(ULONG howmany,  LogicalRegName rd[UARCH_DECODE_WIDTH], RenameTag tdOld[UARCH_DECODE_WIDTH]) { 
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(howmany<=UARCH_DECODE_WIDTH);

  for(ULONG i=0; i<howmany; i++) {
    a2SetMap(rd[i], tdOld[i]);
  }
  return;
}
#endif

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////

RMapSS::RMapSS() {
  rReset();
}

