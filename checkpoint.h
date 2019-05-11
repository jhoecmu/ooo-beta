#ifndef CHECKPOINT_H
#define CHECKPOINT_H
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

#define RESET_CHKPT (0)

bool dependOnSpeculation(SpeculateMask mask, SpeculateMask spec);
bool dependOnSpeculation(SpeculateMask mask, ULONG spec);
bool maskIsSetSpeculation(SpeculateMask spec);
bool maskIsSetOnceSpeculation(SpeculateMask spec);
ULONG whichSpeculation(SpeculateMask spec);

class Checkpoint {
 public:

  SpeculateMask q0GetMask();
  SpeculateMask q0Ground();

  bool q2HasFree();
  ULONG q2NextFree();

  void a2New(ULONG which);

  void a6Free(SpeculateMask);
  void a6Rewind(SpeculateMask);

  void rReset();

  void simTick();

  // Constructor
  Checkpoint();

private:
  SpeculateMask mInuse;
  SpeculateMask mDependOn[UARCH_SPECULATE_DEPTH];
  ULONG mNumInuse;
};

#endif
