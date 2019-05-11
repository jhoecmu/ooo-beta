#define ALU_CPP
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

#include "alu.h"

////////////////////////////////////////////////////////
//
// interface methods
//
////////////////////////////////////////////////////////

AluOut Alu::q6Execute( bool valid, Operation op, ULONG vs1, ULONG vs2, Cookie cookie) {
  AluOut result;

  result.vd=vs1+vs2;
  result.isBr=false;
  result.isMispredict=false;
  result.isException=false;

  if (valid) {
    ASSERT(vs1==cookie.vs1);
    ASSERT(vs2==cookie.vs2);
    ASSERT(result.vd==cookie.vd);

    result.isBr=(op.opcode==BEQ);
    result.isMispredict=((vs1==vs2)!=(op.predTaken==true));

    result.isException=(((popCount(result.vd)%2)==1)!=(op.oparity==true));
  }

  return result;
}
 
void Alu::rReset() { 

  simTick();

  return; 
}                      
void Alu::simTick() { 

  return; 
}                      

////////////////////////////////////////////////////////
//
// Constructors
//
////////////////////////////////////////////////////////
Alu::Alu() {
  rReset();
}


