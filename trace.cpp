#define TRACE_CPP
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

#include "trace.h"
#include "test.h"

static Instruction dummyHalt={.opcode=HALT};

Instruction Trace::getNextTraced() {
  if (mOffset==(sizeof(test)/sizeof(Instruction))) {
    return dummyHalt;
  }
  return test[mOffset++];
}


Instruction Trace::getNextRandom() {
  Instruction inst;

  assert((TRACE_RNAME_RANGE>0)&&
	 (TRACE_RNAME_RANGE<=ARCH_NUM_LOGICAL_REG));

  if (mOffset==TRACE_LENGTH) {
    return dummyHalt;
  }

  {
    ULONG dice=rand()%TRACE_TOTAL;
    
    if (dice<RANDOMIZE(TRACE_ADD_SHARE)) {
      inst.opcode=ADD;
    } else {
      inst.opcode=BEQ;
    }
  }


#define REGDICE (rand()%TRACE_RNAME_RANGE)
#define REGDRIFT ((mOffset*TRACE_DRIFT_MUL)/TRACE_DRIFT_DIV)


  if (inst.opcode==BEQ) {
    inst.rd=R0;
  } else {
    inst.rd=(LogicalRegName)(TRACE_WITH_R0?
			     ((REGDICE+REGDRIFT)%ARCH_NUM_LOGICAL_REG):
			     (1+((REGDICE+REGDRIFT)%(ARCH_NUM_LOGICAL_REG-1))));
  }

  inst.rs1=(LogicalRegName)(TRACE_WITH_R0?
			   ((REGDICE+REGDRIFT)%ARCH_NUM_LOGICAL_REG):
			    (1+((REGDICE+REGDRIFT)%(ARCH_NUM_LOGICAL_REG-1))));
  inst.rs2=(LogicalRegName)(TRACE_WITH_R0?
			    ((REGDICE+REGDRIFT)%ARCH_NUM_LOGICAL_REG):
			    (1+((REGDICE+REGDRIFT)%(ARCH_NUM_LOGICAL_REG-1))));

  if (inst.opcode==BEQ) {
    ULONG dice=rand()%TRACE_BR_HITMISS;
    
    if (dice<RANDOMIZE(TRACE_BR_HIT)) {
      inst.miss=false;
    } else {
      inst.miss=true;
    }

  } else {
    inst.miss=false;
  }

  {
    ULONG dice=rand()%TRACE_EXCEPT_TOTAL;
    if (dice<TRACE_EXCEPT) {
      inst.exception=true;
    } else {
      inst.exception=false;
    }
  }


  mOffset++;

  return inst;
}

void Trace::rReset() {
 this->mOffset=0;
}

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Trace::Trace() {
  cout << "TRACE_RANDOM=" << TRACE_RANDOM << "\n";
  cout << "TRACE_WITH_R0=" << TRACE_WITH_R0 << "\n";
  cout << "TRACE_RNAME_RANGE=" << TRACE_RNAME_RANGE << "\n";
  cout << "TRACE_DRIFT_DIV=" << TRACE_DRIFT_DIV << "\n";
  cout << "TRACE_DRIFT_MUL=" << TRACE_DRIFT_MUL << "\n";
  cout << "TRACE_LENGTH=" << TRACE_LENGTH << "\n";
  cout << "TRACE_ADD_SHARE=" << TRACE_ADD_SHARE << "\n";
  cout << "TRACE_BR_SHARE=" << TRACE_BR_SHARE << "\n";
  cout << "TRACE_TOTAL=" << TRACE_TOTAL << "\n";
  cout << "TRACE_BR_HIT=" << TRACE_BR_HIT << "\n";
  cout << "TRACE_BR_MISS=" << TRACE_BR_MISS << "\n";
  cout << "TRACE_BR_HITMISS=" << TRACE_BR_HITMISS << "\n";
  cout << "TRACE_EXCEPT=" << TRACE_EXCEPT << "\n";
  cout << "TRACE_EXCEPT_TOTAL=" << TRACE_EXCEPT_TOTAL << "\n";
  
  rReset();
}
