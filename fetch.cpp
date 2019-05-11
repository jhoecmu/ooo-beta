#define FETCH_CPP
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

#include "fetch.h"



FetchBundle Fetch::qGetInsts() {
  for(ULONG i=mBundle.howmany; i<UARCH_DECODE_WIDTH; i++) {
    Biscuit biscuit;
#if (TRACE_RANDOM)
    Instruction ir=mTrace.getNextRandom();
#else
    Instruction ir=mTrace.getNextTraced();
#endif

    if (ir.opcode==HALT) break;

    mBundle.howmany++;

    mBundle.inst[i]=ir;

    biscuit=mMagic.aFunctional(ir);
#if (DEBUG_LEVEL>=DEBUG_SILENT)
    mBundle.cookie[i].serial=biscuit.serial;
    mBundle.cookie[i].vd=biscuit.vd;
    mBundle.cookie[i].vs1=biscuit.vs1;
    mBundle.cookie[i].vs2=biscuit.vs2;
    mBundle.cookie[i].inst=biscuit.inst;
    mBundle.cookie[i].op=biscuit.op;
    mBundle.cookie[i].speculating=biscuit.speculating;
#endif
    mBundle.pcLike[i]=biscuit.serial;

    mBundle.predTaken[i]=(mBundle.inst[i].miss==true)!=(biscuit.vs1==biscuit.vs2);
    mBundle.oparity[i]=(mBundle.inst[i].exception==true)!=((popCount(biscuit.vd)%2)==1);

    // this information not available to datapath; cookie has a backup copy though
    mBundle.inst[i].miss=true;
    mBundle.inst[i].exception=true;
  }

  return mBundle;
}

void Fetch::aAccept(ULONG n) {
  assert(n<=UARCH_DECODE_WIDTH);
  assert(n<=mBundle.howmany);

  for(ULONG i=0, j=n; j<mBundle.howmany; i++,j++) {
    mBundle.inst[i]=mBundle.inst[j];
    mBundle.cookie[i]=mBundle.cookie[j];
    mBundle.pcLike[i]=mBundle.pcLike[j];
    mBundle.predTaken[i]=mBundle.predTaken[j];
    mBundle.oparity[i]=mBundle.oparity[j];
  }

  mBundle.howmany=mBundle.howmany-n;
}

void Fetch::aRewind(ULONG serial) {
#if (DEBUG_LEVEL>=DEBUG_FULL)
  cout << "cyc" << (simTimer/TICK_CYC) << " ******************* rewinding to s" << serial << " continue from s" << mMagic.qSerial() << "*******************\n";
#endif
  mMagic.aRewind(serial);
  mBundle.howmany=0;
}

void Fetch::aRestart(ULONG serial) {
#if (DEBUG_LEVEL>=DEBUG_FULL)
  cout << "cyc" << (simTimer/TICK_CYC) << " ******************* restarting to s" << serial << " continue from s" << mMagic.qSerial() << "*******************\n";
#endif
  mMagic.aRestart(serial);
  mBundle.howmany=0;
}

void Fetch::rReset() {
  mMagic.rReset();
  mTrace.rReset();

  mBundle.howmany=0;
}

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Fetch::Fetch() {
  rReset();
}

