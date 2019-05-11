#define PRINT_CPP
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

#include "checkpoint.h"

void printMask(SpeculateMask mask) {
  cout << " ";
  FOR_SPECULATE_DEPTH_i {
    cout << (mask.bit[i]?"1":"0");
  }
}


Operation NULLOP;

void prettyPrint(const char stage[], Cookie cookie) {
  prettyPrint(stage, NULLOP, cookie, "", "");
}

void prettyPrint(const char stage[], Operation op, Cookie cookie) {
  prettyPrint(stage, op, cookie, "", "");
}

void prettyPrint(const char stage[], Operation op, Cookie cookie, const char prefix[], const char suffix[]) {
#if (DEBUG_LEVEL>=DEBUG_FULL) 

  if ((simTimer/TICK_CYC)%DEBUG_PRINT_DOWNSAMPLE) { return; }

  cout << prefix ;
  cout << "cyc" << (simTimer/TICK_CYC) << stage << "s" << cookie.serial << "(" << (cookie.speculating) << ")";
  cout << OpCodeString[cookie.inst.opcode];
  if (cookie.inst.opcode==BEQ) {
    cout << "(";
    if (cookie.inst.miss) {
      cout << "m";
    }
    cout << op.checkpoint << ")";
  }
  if (cookie.inst.exception) {
    cout << "Ex";
  }
  cout << " rd=R" << cookie.inst.rd 
       << " rs1=R" << cookie.inst.rs1 
       << " rs2=R" << cookie.inst.rs2  ;
  cout << " :: td=t" << tagToPRegIdx(op.td) 
       << " ts1=t" << tagToPRegIdx(op.ts1) 
       << " ts2=t" << tagToPRegIdx(op.ts2) ;
  printMask(op.dependOn);
  cout << suffix ;
  cout << "\n";
#endif
}  
