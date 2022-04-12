#ifndef TEST_H
#define TEST_H
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

// small example of register data dependence, without control flow

static Instruction test[]={
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2}
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
  {.opcode=ADD, .rd=R0, .rs1=R5, .rs2=R4},
  {.opcode=ADD, .rd=R1, .rs1=R0, .rs2=R5},
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R0},
  {.opcode=ADD, .rd=R3, .rs1=R2, .rs2=R1},
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R2},
};

#endif
