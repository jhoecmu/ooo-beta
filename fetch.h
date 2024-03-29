#ifndef FETCH_H
#define FETCH_H
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

#include "trace.h"

typedef struct {
  ULONG howmany;
  Instruction inst[UARCH_DECODE_WIDTH];
  ULONG pcLike[UARCH_DECODE_WIDTH];
  bool predTaken[UARCH_DECODE_WIDTH];
  bool oparity[UARCH_DECODE_WIDTH];
  Cookie cookie[UARCH_DECODE_WIDTH];
} FetchBundle;

class Fetch {
 public:
  FetchBundle qGetInsts();
  
  void aAccept(ULONG n);
  void aRewind(ULONG serial);
  void aRestart(ULONG serial);

  void rReset();

  void simTick();

  // Constructor
  Fetch();

 private:
  Trace mTrace;
  Magic mMagic;

  FetchBundle mBundle;
};


#endif
