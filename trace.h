#ifndef TRACE_H
#define TRACE_H
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

#define TRACE_RANDOM (1)
#define TRACE_USE_BASELINE (0)

#if (!TRACE_USE_BASELINE)
// for hacking

#define TRACE_WITH_R0     (1)
#define TRACE_RNAME_RANGE (2)
#define TRACE_DRIFT_DIV   (4)
#define TRACE_DRIFT_MUL   (1)
#define TRACE_LENGTH      (100000)

#define TRACE_ADD_SHARE (3)
#define TRACE_BR_SHARE  (1)
#define TRACE_TOTAL     (TRACE_ADD_SHARE+TRACE_BR_SHARE)

#define TRACE_BR_HIT     (2)
#define TRACE_BR_MISS    (1)
#define TRACE_BR_HITMISS (TRACE_BR_HIT+TRACE_BR_MISS)

#define TRACE_EXCEPT       (2)
#define TRACE_EXCEPT_TOTAL (500)

#define RANDOMIZE(a) (a)
//#define RANDOMIZE(a) ((ULONG)(rand()%(a)))

#else
// for regression

#define TRACE_WITH_R0     (1)
#define TRACE_RNAME_RANGE (2)
#define TRACE_DRIFT_DIV   (4)
#define TRACE_DRIFT_MUL   (1)
#define TRACE_LENGTH      (10000000)

#define TRACE_ADD_SHARE (6)
#define TRACE_BR_SHARE  (1)
#define TRACE_TOTAL     (TRACE_ADD_SHARE+TRACE_BR_SHARE)

#define TRACE_BR_HIT     (9)
#define TRACE_BR_MISS    (1)
#define TRACE_BR_HITMISS (TRACE_BR_HIT+TRACE_BR_MISS)

#define TRACE_EXCEPT       (2)
#define TRACE_EXCEPT_TOTAL (500)

#define RANDOMIZE(a) (a)

#endif

class Trace {
 public:
  Instruction getNextTraced();
  Instruction getNextRandom();
  
  void rReset();

  void simTick();

  // Constructor
  Trace();

 private:
  ULONG mOffset;
};

#endif
