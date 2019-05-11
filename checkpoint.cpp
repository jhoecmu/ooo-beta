#define CHECKPOINT_CPP
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

#include "checkpoint.h"

bool dependOnSpeculation(SpeculateMask mask, SpeculateMask spec) {
  bool dependOn=false;

  FOR_SPECULATE_DEPTH_i {
    if ((mask.bit[i])&&(spec.bit[i])) {
      dependOn=true;
    }
  }

  return dependOn;
}   

bool dependOnSpeculation(SpeculateMask mask, ULONG spec) {
  return mask.bit[spec];
}


bool maskIsSetSpeculation(SpeculateMask spec) {
  bool isSet=false;

  FOR_SPECULATE_DEPTH_i {
    if (spec.bit[i]) {
      isSet=true;
    }
  }

  return isSet;
}   

ULONG whichSpeculation(SpeculateMask spec) {
  ASSERT(maskIsSetOnceSpeculation(spec));
  ULONG which=0;

  FOR_SPECULATE_DEPTH_i {
    if (spec.bit[i]) {
      which=i;
    }
  }

  return which;
}

bool maskIsSetOnceSpeculation(SpeculateMask spec) {
  ULONG isSet=0;

  FOR_SPECULATE_DEPTH_i {
    if (spec.bit[i]) {
      isSet++;
    }
  }

  return (isSet==1);
}   

bool Checkpoint::q2HasFree() {
  USAGEWARN((!simTock), "query after TOCK");

  return (mNumInuse<UARCH_SPECULATE_DEPTH);
}

ULONG Checkpoint::q2NextFree() {
  USAGEWARN((!simTock), "query after TOCK");

  ASSERT(mNumInuse<UARCH_SPECULATE_DEPTH);

  ULONG next;

  for(next=0; next<UARCH_SPECULATE_DEPTH; next++) {
    if (mInuse.bit[next]==false) {
      break;
    }
  }
  ASSERT(next<UARCH_SPECULATE_DEPTH);
  
  return next;
}

SpeculateMask Checkpoint::q0GetMask() {
  USAGEWARN((!simTock), "query after TOCK");

  SpeculateMask mask=mInuse;

  return mask;
}


SpeculateMask Checkpoint::q0Ground() {
  USAGEWARN((!simTock), "query after TOCK");
  SpeculateMask mask;
  FOR_SPECULATE_DEPTH_i { mask.bit[i]=false; }

  FOR_SPECULATE_DEPTH_i {
    if (mInuse.bit[i]&&(!maskIsSetSpeculation(mDependOn[i]))) {
      mask.bit[i]=true;
    }
  }

  if (maskIsSetSpeculation(mask)) {
    // if speculation, found the OLDEST one
    ASSERT(maskIsSetOnceSpeculation(mask));
  }

  return mask;
}

void Checkpoint::a2New(ULONG next) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(mNumInuse<UARCH_SPECULATE_DEPTH);
  ASSERT(next<UARCH_SPECULATE_DEPTH);
  ASSERT(mInuse.bit[next]==false);

  mDependOn[next]=mInuse;
  mInuse.bit[next]=true;
  mNumInuse++;
}

void Checkpoint::a6Free(SpeculateMask mask) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(mNumInuse>=1);
  ASSERT(maskIsSetOnceSpeculation(mask));

  {
    FOR_SPECULATE_DEPTH_i {
      if (mask.bit[i]) {
	ASSERT(mInuse.bit[i]);
	mInuse.bit[i]=false;
      }
    }
  }
  ASSERT(mNumInuse>=1);
  mNumInuse--;

  {
    FOR_SPECULATE_DEPTH_i {
      if (mInuse.bit[i]) { // okay to not check this
	FOR_SPECULATE_DEPTH_j {
	  if ((mDependOn[i].bit[j]) && mask.bit[j]) {
	    mDependOn[i].bit[j]=false;
	  }
	}
      }
    }
  }
}

void Checkpoint::a6Rewind(SpeculateMask mask) {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(mNumInuse>0);
  ASSERT(maskIsSetOnceSpeculation(mask));

  FOR_SPECULATE_DEPTH_i {
    if (mInuse.bit[i]) { // okay to not check this
      ASSERT(mNumInuse>=1);
      
      if(dependOnSpeculation(mDependOn[i],mask)) {
	mInuse.bit[i]=false;
	mNumInuse--;
	ASSERT(mNumInuse>=1);
      }
    }
  }

  FOR_SPECULATE_DEPTH_i {
    if (mask.bit[i]) {
      ASSERT(mInuse.bit[i]);
      mInuse.bit[i]=false;
    }
  }
  ASSERT(mNumInuse>=1);
  mNumInuse--;
}

void Checkpoint::rReset() {
  simTick();

  FOR_SPECULATE_DEPTH_i {
    mInuse.bit[i]=false;
  }

  mNumInuse=0;
}

void Checkpoint::simTick() {
}

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Checkpoint::Checkpoint() {
  cout << "RESET_CHKPT=" << RESET_CHKPT << "\n";

  rReset();
}

