#define INSTQ_CPP
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

#include "instq.h"
#include "checkpoint.h"

////////////////////////////////////////////////////////
//
// interface methods
//
////////////////////////////////////////////////////////

ULONG InstQ::q2NumSlots() {
  USAGEWARN((!simTock), "query after TOCK");

  ASSERT (mInUse<=UARCH_INSTQ_SIZE);

  return (UARCH_INSTQ_SIZE-mInUse);
}

InstQEntry InstQ::q4Readied() {
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumReadied++)<MAX_INSTQ_READY), "exceeding number of InstQ ready CAM-read port limit\n");
  printState();

  LONG which=-1;
  mScan=MSCANSTART;
  if (mInUse) {
    FOR_INSTQ_SIZE_i {
      if ((mArray[mScan].valid) &&
	  (mArray[mScan].ts1Ready) &&
	  (mArray[mScan].ts2Ready)) {
	which=mScan; 
	break;
      }
      mScan++;
      mScan%=UARCH_INSTQ_SIZE;
    } 
  }

  if (which==-1) {
    return invalidEntryInstQ;
  } else {
    ASSERT(mInUse);
    return mArray[which];
  }
}


void InstQ::a3Insert(ULONG atag, Operation op, 
		     bool ts1Busy, bool ts2Busy, 
		     Cookie cookie) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumInsert++)<MAX_INSTQ_INSERT), "exceeding number of InstQ insert write port limit\n");

  ASSERT(mInUse<(UARCH_INSTQ_SIZE));

  mScan=MSCANSTART;
  FOR_INSTQ_SIZE_i {
    if (!mArray[mScan].valid) {
      mInUse++;
      mArray[mScan].valid=1;
      
      mArray[mScan].atag=atag;
      mArray[mScan].op=op;
      mArray[mScan].ts1Ready=!ts1Busy;
      mArray[mScan].ts2Ready=!ts2Busy;
      mArray[mScan].cookie=cookie;
      mScan++;
      mScan%=UARCH_INSTQ_SIZE;
      return;
    }
    mScan++;
    mScan%=UARCH_INSTQ_SIZE;
  } 
  ASSERT(0);
}

void InstQ::a4Issue(ULONG which) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumIssue++)<MAX_INSTQ_ISSUE), "exceeding number of InstQ issue set port limit\n");

  ASSERT(which<UARCH_INSTQ_SIZE);
  ASSERT(mArray[which].valid);
  ASSERT(mInUse);

  prettyPrint(ISTAGE,mArray[which].op,mArray[which].cookie);
  mArray[which].valid=0;
  mInUse--;
}

void InstQ::a4Release(RenameTag which, Cookie cookie) {

  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumRelease++)<MAX_INSTQ_RELEASE), "exceeding number of InstQ release CAM-write  port limit\n");

  //cout << "rforwarding t" << which.idx << "\n"; 

  if (!tagEqual(which, ZeroRegTag)) {
    FOR_INSTQ_SIZE_i {
      if (tagEqual(mArray[i].op.ts1,which)) { 
	if (mArray[i].valid) {
	  ASSERT(mArray[i].cookie.serial>cookie.serial);
	  ASSERT(!mArray[i].ts1Ready);
	}
	mArray[i].ts1Ready=true;
      }
      if (tagEqual(mArray[i].op.ts2,which)) {
	if (mArray[i].valid) {
	  ASSERT(mArray[i].cookie.serial>cookie.serial);
	  ASSERT(!mArray[i].ts2Ready);
	}
	mArray[i].ts2Ready=true;
      }
    } 
  }
}

void InstQ::a6Squash(SpeculateMask mask, Cookie cookie) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumSquash++)<MAX_INSTQ_SQUASH), "exceeding number of InstQ squash CAM-clear port limit\n");

  ASSERT(maskIsSetOnceSpeculation(mask));
	 
  FOR_INSTQ_SIZE_i {
    if (mArray[i].valid) {
      if (dependOnSpeculation(mArray[i].op.dependOn, mask)) {
	ASSERT(mArray[i].cookie.serial>cookie.serial);
	// squashed
	prettyPrint(IkSTAGE,mArray[i].op,mArray[i].cookie);
	mArray[i].valid=false;
	mInUse--;
      }
    } 
  }
}

void InstQ::a6ClearMask(SpeculateMask mask, Cookie cookie) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumClear++)<MAX_INSTQ_CLEAR), "exceeding number of InstQ clear port CAM-clear limit\n");

  ASSERT(maskIsSetOnceSpeculation(mask));
	 
  FOR_INSTQ_SIZE_i {
    FOR_SPECULATE_DEPTH_j {
      if (mArray[i].op.dependOn.bit[j] && mask.bit[j]) {
	if (mArray[i].valid) {
 	  ASSERT(mArray[i].cookie.serial>cookie.serial);
	}
	mArray[i].op.dependOn.bit[j]=false;
      }
    } 
  }
}

#if (UARCH_ROB_RENAME) 
void InstQ::a7retireTag(RenameTag ptag, RenameTag ltag, Cookie cookie) {

  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumRetire++)<MAX_INSTQ_RETIRE), "exceeding number of InstQ retire CAM-write  port limit\n");

  if (!tagEqual(ptag, ZeroRegTag)) {
    FOR_INSTQ_SIZE_i {
      if (tagEqual(mArray[i].op.ts1,ptag)) {
	if (mArray[i].valid) {
	  ASSERT(mArray[i].cookie.serial>cookie.serial);
	}
	mArray[i].op.ts1=ltag;
      }
      if (tagEqual(mArray[i].op.ts2,ptag)) {
	if (mArray[i].valid) {
	  ASSERT(mArray[i].cookie.serial>cookie.serial);
	}
	mArray[i].op.ts2=ltag;
      }
    } 
  }
}
#endif

void InstQ::printState() {
#if (DEBUG_LEVEL>=DEBUG_VERBOSE)
  {
    bool printing=false;
    
    if (mInUse) {
      FOR_INSTQ_SIZE_i {
	if (!printing) {
	  cout << "------------------------------------------------------------------------------------\n";
	  printing=true;
	}
	if (mArray[i].valid) {
	  if ((mArray[i].ts1Ready) &&
	      (mArray[i].ts2Ready)) {
	    prettyPrint(":InINSTQ:", mArray[i].op, mArray[i].cookie, "\t\t", " Readied");
	  } else if ((mArray[i].ts1Ready) ||
		     (mArray[i].ts2Ready)) {
	    prettyPrint(":InINSTQ:", mArray[i].op, mArray[i].cookie, "\t\t", mArray[i].ts1Ready?" 10":" 01");
	  } else {
	    prettyPrint(":InINSTQ:", mArray[i].op, mArray[i].cookie, "\t\t", "");
	  }
	  //cout << "\t\t\t\t" << mArray[i].vs1 << "::" << mArray[i].vs2 << "\n";
	}
      }
    }
    if (printing) {
      cout << "------------------------------------------------------------------------------------\n";
    }
  }
#endif
}

void InstQ::rReset() { 
  simTick();

  mInUse=0;
  mScan=0;
  //mNoBody.slotNum=0;
  //mNoBody.valid=false;
  //mNoBody.op.td=0;
  //mNoBody.op.ts1=0;
  //mNoBody.op.ts2=0;

  FOR_INSTQ_SIZE_i  {
    mArray[i].slotIdx=i;
    mArray[i].valid=0;
  }

  return; 
}                      
void InstQ::simTick() { 
  dNumReadied=0;
  dNumInsert=0;
  dNumIssue=0;
  dNumRelease=0;
  dNumRetire=0;
  dNumSquash=0;
  dNumClear=0;

  return; 
}                      

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
InstQ::InstQ() {
  cout << "MAX_INSTQ_READY=" << MAX_INSTQ_READY << "\n";
  cout << "MAX_INSTQ_INSERT=" << MAX_INSTQ_INSERT << "\n";
  cout << "MAX_INSTQ_ISSUE=" << MAX_INSTQ_ISSUE << "\n";
  cout << "MAX_INSTQ_RELEASE=" << MAX_INSTQ_RELEASE << "\n";
  cout << "MAX_INSTQ_RETIRE=" << MAX_INSTQ_RETIRE << "\n";
  cout << "MAX_INSTQ_SQUASH=" << MAX_INSTQ_SQUASH << "\n";
  cout << "MAX_INSTQ_CLEAR=" << MAX_INSTQ_CLEAR << "\n";
  cout << "INSTQ_MSCAN_RANDOM=" << INSTQ_MSCAN_RANDOM << "\n";
  cout << "INSTQ_MSCAN_RROBIN=" << INSTQ_MSCAN_RROBIN << "\n";

  rReset();
}



