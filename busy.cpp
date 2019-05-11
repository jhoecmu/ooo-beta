#define BUSY_CPP
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

#include "busy.h"

////////////////////////////////////////////////////////
//
// interface methods
//
////////////////////////////////////////////////////////

// lookup logical to physical mapping
bool Busy::q3IsBusy(PhysicalRegIdx preg) { 
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumRead++)<MAX_BUSY_READ), "exceeding number of Busy Table read port limit\n");

  ASSERT(preg<UARCH_NUM_PHYSICAL_REG);

  bool isBusy;

  if (preg!=0) {
    isBusy=mArray[preg];
  } else {
    isBusy=false;
  }

  return isBusy;
}


// set new logical to physical mapping
void Busy::a2SetBusy(PhysicalRegIdx preg) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumSet++)<MAX_BUSY_SET), "exceeding number of Busy Table set port limit\n");

  ASSERT(preg<UARCH_NUM_PHYSICAL_REG);

  if (preg) {
    mArray[preg]=true;
  }

  return;
}

void Busy::a4ClearBusy(PhysicalRegIdx preg) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumClear++)<MAX_BUSY_CLEAR), "exceeding number of Busy Table clear port limit\n");

  ASSERT(preg<UARCH_NUM_PHYSICAL_REG);

  if (preg) {
    mArray[preg]=false;
  }

  return;
}


void Busy::rReset() { 
  simTick();

  for(ULONG i=0; i<UARCH_NUM_PHYSICAL_REG; i++)  {
    mArray[i]=false;
  }

  return; 
}                      
void Busy::simTick() { 

  dNumRead=0;
  dNumSet=0;
  dNumClear=0;

  return; 
}                      

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Busy::Busy() {
  cout << "MAX_BUSY_READ=" << MAX_BUSY_READ << "\n";
  cout << "MAX_BUSY_SET=" << MAX_BUSY_SET << "\n";
  cout << "MAX_BUSY_CLEAR=" << MAX_BUSY_CLEAR << "\n";

  rReset();
}


