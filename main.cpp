#define MAIN_CPP
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

#include "datapath.h"

FetchBundle nothing={.howmany=0};

int main(int argc, char *argv[]) {
  ULONG countdown=UARCH_OOO_DEGREE*2;
  ULONG cycle=0;
  ULONG instCount=0;

  //----------------------------------------------------
  //
  // instantiate datapath objects
  // 
  //----------------------------------------------------
  Fetch fetch;

  FetchBundle fetchedInsts;
  ULONG accept;
  bool rewind;
  bool restart;
  ULONG gotoPC;

  fetch.rReset();
  datapath(true, nothing, &accept, &rewind, &restart, &gotoPC );

  while(1) {
    fetchedInsts=fetch.qGetInsts();

    datapath(false, fetchedInsts, &accept, &rewind, &restart, &gotoPC );
    instCount+=accept;
    
    fetch.aAccept(accept);
    if (rewind) {
      ASSERT(!restart);
      fetch.aRewind(gotoPC);
    }
    if (restart) {
      ASSERT(!rewind);
      fetch.aRestart(gotoPC);
    }

    if (fetchedInsts.howmany==0) {
      if ((--countdown)==0) {
	break;
      }
    }

    // advance time
    simTimer+=TICK_CYC;
    cycle++;
  }

  cout << "Exiting: " << cycle << " cycles; " << instCount << " instructions completed.\n";

  return 0;
}
