#define EXCEPTION_CPP
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

#include "exception.h"
#include "checkpoint.h"

bool Exception::q0Pending() {
  USAGEWARN((!simTock), "query after TOCK");

  return mPending;
}

void Exception::a6Raise(SpeculateMask mask, Cookie cookie ) {
  USAGEWARN(simTock, "action before TOCK");

  ULONG old=0, next=0;

  if (!mPending) {
    mPending=true;
    mDependOn=mask;
    mCookie=cookie;
    ASSERT(cookie.inst.exception);
    return;
  }

  FOR_SPECULATE_DEPTH_i {
    old+=mDependOn.bit[i]?1:0;
    next+=mask.bit[i]?1:0;
  }

  if (next<old) {
    ASSERT(cookie.inst.exception);
    ASSERT(cookie.serial<mCookie.serial);
    mDependOn=mask;
    mCookie=cookie;
  }
#if (DEBUG_LEVEL>=DEBUG_SILENT)
  if (next==old) {
    // all else being equal, we cheat during debug to register the older of the two
    if (cookie.serial<mCookie.serial) {
      ASSERT(cookie.inst.exception);
      mDependOn=mask;
      mCookie=cookie;
    }
  }
#endif

  {
    FOR_SPECULATE_DEPTH_i {
      if (next<=old) {
	if (mask.bit[i]) {
	  ASSERT(mDependOn.bit[i]);
	}
      } else {
	if (mDependOn.bit[i]) {
	  ASSERT(mask.bit[i]);
	}
      }
    }
  }
}

void Exception::a6Cancel(SpeculateMask mask, Cookie cookie) {
  USAGEWARN(simTock, "action before TOCK");

  if (mPending) {  // okay to not check this;
    FOR_SPECULATE_DEPTH_i {
      if (mDependOn.bit[i] && mask.bit[i]) {
	mPending=false;
	ASSERT(cookie.serial<mCookie.serial);
	ASSERT(mCookie.speculating);
	break;
      }
    }
  }
}

void Exception::a6ClearMask(SpeculateMask mask, Cookie cookie) {
  USAGEWARN(simTock, "action before TOCK");

  if (mPending) {  // okay to not check this;
    FOR_SPECULATE_DEPTH_i {
      if (mDependOn.bit[i] && mask.bit[i]) {
	mDependOn.bit[i]=false;
	ASSERT(cookie.serial<mCookie.serial);
	break;
      }
    }
  }
}

void Exception::a0ClearPending() {
  USAGEWARN(simTock, "action before TOCK");

  ASSERT(mPending);
  ASSERT(mCookie.speculating==0);
  ASSERT(!maskIsSetSpeculation(mDependOn));
  rReset();
}


void Exception::rReset() {

  simTick();

  FOR_SPECULATE_DEPTH_i {
    mDependOn.bit[i]=false;
  }

  mPending=false;
}

void Exception::simTick() {
}

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Exception::Exception() {
  rReset();
}

