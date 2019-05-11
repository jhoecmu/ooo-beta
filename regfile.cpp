#define REGFILE_CPP
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

#include "regfile.h"


////////////////////////////////////////////////////////
//
// interface methods
//
////////////////////////////////////////////////////////

DataValue RegFile::q5Read(PhysicalRegIdx preg) { 
  USAGEWARN((!simTock), "query after TOCK");
  USAGEWARN(((dNumRead++)<MAX_REGFILE_READ), "exceeding number of Regfile read port limit\n");

  ASSERT(preg<UARCH_NUM_PHYSICAL_REG);

  DataValue val;

  if (preg!=0) {
    val=mArray[preg];
  } else {
    ASSERT(mArray[0]==0);
    val=0;
  }

  return val;
}


void RegFile::a6Write(PhysicalRegIdx preg, DataValue val) {
  USAGEWARN(simTock, "action before TOCK");
  USAGEWARN(((dNumWrite++)<MAX_REGFILE_WRITE), "exceeding number of Regfile write port limit\n");

  ASSERT(preg<UARCH_NUM_PHYSICAL_REG);

  if (preg!=0) {
    mArray[preg]=val;
  }

  return;
}

void RegFile::rReset() { 
  simTick();

  for(ULONG i=0; i<UARCH_NUM_PHYSICAL_REG; i++)  {
    mArray[i]=i;
  }
  mArray[0]=0;

  return; 
}                      
void RegFile::simTick() { 

  dNumRead=0;
  dNumWrite=0;

  return; 
}                      

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
RegFile::RegFile() {
  cout << "MAX_REGFILE_READ=" << MAX_REGFILE_READ << "\n";
  cout << "MAX_REGFILE_WRITE=" << MAX_REGFILE_WRITE << "\n";

  rReset();
}

