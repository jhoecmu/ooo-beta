#ifndef MAGIC_H
#define MAGIC_H
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

typedef struct{
#if (DEBUG_LEVEL>=DEBUG_SILENT)
  ULONG serial;
  DataValue vd, vs1, vs2;
  Instruction inst;
  Operation op;
  ULONG speculating;
#endif
} Cookie;

typedef struct{
  ULONG serial;
  DataValue vd, vs1, vs2;
  Instruction inst;
  Operation op;
  ULONG speculating;
} Biscuit;


typedef struct{
  ULONG serial;
  LogicalRegName rd;
  DataValue val;
  bool isMiss;
  bool isException;
} ReplayLog;

class Magic {
 public:

  ULONG qSerial();

  Biscuit aFunctional(Instruction inst);
  void aRewind(ULONG serial);
  void aRestart(ULONG serial);

  void rReset();

  void simTick();

  // Constructor
  Magic();

 private:
  ULONG mSerial;
  ULONG mSpeculating;

  ReplayLog log[UARCH_OOO_DEGREE+UARCH_DECODE_WIDTH];
  DataValue mRF[ARCH_NUM_LOGICAL_REG];
};


#endif
