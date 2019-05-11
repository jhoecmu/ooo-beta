#define MAGIC_CPP
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

ULONG Magic::qSerial() {
  return mSerial;
}

Biscuit Magic::aFunctional(Instruction inst) {
  Biscuit biscuit;

  biscuit.serial=mSerial++;
  biscuit.speculating=mSpeculating;
  if (mSpeculating) {
    mSpeculating++;
  }

  assert(biscuit.speculating<(UARCH_OOO_DEGREE+UARCH_DECODE_WIDTH));

  // overwrite entry 0 if not speculating
  log[biscuit.speculating].serial=biscuit.serial;
  log[biscuit.speculating].rd=inst.rd;
  log[biscuit.speculating].val=mRF[inst.rd];
  log[biscuit.speculating].isMiss=false;
  log[biscuit.speculating].isException=false;

  biscuit.inst=inst;
  biscuit.vs1=inst.rs1?mRF[inst.rs1]:0;
  biscuit.vs2=inst.rs2?mRF[inst.rs2]:0;
  biscuit.vd=biscuit.vs1+biscuit.vs2;

  if (inst.exception) {
    log[biscuit.speculating].isException=true;
    if (mSpeculating==0) {
      // beginning of wrongpath
      mSpeculating=1;
    }
  }

  switch(inst.opcode)  {
  case ADD: 
    if(inst.rd) {
      mRF[inst.rd]=biscuit.vd;
    }
#if (DEBUG_LEVEL==DEBUG_TRACE)
    if (!mSpeculating) {
      cout << "magic[" << biscuit.serial << "] RF[" << inst.rd << "]<=" << biscuit.vd << "\n";
    }
#endif
    break;
  case BEQ:
    assert(inst.rd==R0);
#if (DEBUG_LEVEL==DEBUG_TRACE)
    if (!mSpeculating) {
      cout << "magic[" << biscuit.serial << "] BR@" << biscuit.vd << "\n";
    }
#endif
    if (inst.miss) {
      log[biscuit.speculating].isMiss=true;
      if (mSpeculating==0) {
	// beginning of wrongpath
	mSpeculating=1;
      }
    }
    break;
  default:
    assert(0);
  }
  return biscuit;
};

void Magic::aRewind(ULONG serial) {
  assert(mSpeculating);
  bool found=false;

  while(mSpeculating--) {
    if (serial==log[mSpeculating].serial) {
      assert(log[mSpeculating].isMiss);
      log[mSpeculating].isMiss=false;
      found=true;
      break;
    }
    if (log[mSpeculating].rd!=R0) {
      mRF[log[mSpeculating].rd]=log[mSpeculating].val;
    } else {
      assert(log[mSpeculating].val==0);
      assert(mRF[R0]==0);
    }
  }

  assert(found);

  if (mSpeculating==0) {
    if (log[0].isException) {
      mSpeculating=1;
    }
  }
  return;
}


void Magic::aRestart(ULONG serial) {
  bool found=false;
  assert(mSpeculating);

  while(mSpeculating--) {
    if (log[mSpeculating].rd!=R0) {
      mRF[log[mSpeculating].rd]=log[mSpeculating].val;
    } else {
      assert(log[mSpeculating].val==0);
      assert(mRF[R0]==0);
    }
    if (serial==log[mSpeculating].serial) {
      assert(mSpeculating==0);
      assert(log[mSpeculating].isException);
      found=true;
      break;
    }
  }
  assert(found);

  return;
}

void Magic::rReset() {
  mSerial=0; 
  mSpeculating=0;

  for(ULONG i=0;i<ARCH_NUM_LOGICAL_REG;i++) {
    mRF[i]=i;
  }
}

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Magic::Magic() {
  rReset();
}

