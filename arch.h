#ifndef ARCH_H
#define ARCH_H
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

#define ARCH_NUM_LOGICAL_REG (32)

typedef ULONG DataValue;  // operand value data type

// See README and [Hoe, "Superscalar Out-of-Order Demystified in Four
// Instructions," 2003.
// http://users.ece.cmu.edu/~jhoe/distribution/2003/wcae03.pdf.]
enum OpCode {
  ADD,
  BEQ,
  HALT,
  DONTCARE
};

#ifdef MAIN_CPP
const char *OpCodeString[]={ 
  "ADD",
  "BEQ",
  "HALT"
  "*"
};
#else
extern char *OpCodeString[];
#endif

enum LogicalRegName {
   R0,  R1,  R2,  R3,  R4,  R5,  R6,  R7,  R8,  R9,
  R10, R11, R12, R13, R14, R15, R16, R17, R18, R19,
  R20, R21, R22, R23, R24, R25, R26, R27, R28, R29,
  R30, R31
};

typedef struct {
  OpCode opcode;
  LogicalRegName rd, rs1, rs2; 
  bool miss;   // if instruction is a BP misprediction or a cache miss as appropriate 
  bool exception;
} Instruction;

#endif
