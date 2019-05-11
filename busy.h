#ifndef BUSY_H
#define BUSY_H
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

#define MAX_BUSY_READ (UARCH_DECODE_WIDTH*2)
#define MAX_BUSY_SET (UARCH_DECODE_WIDTH)
#define MAX_BUSY_CLEAR (UARCH_EXECUTE_WIDTH)

class Busy {
 public:
  //
  // external interfaces
  //
  bool q3IsBusy(PhysicalRegIdx preg);

  void a2SetBusy(PhysicalRegIdx preg);

  void a4ClearBusy(PhysicalRegIdx preg);

  void rReset();

  void simTick();

  // Constructor
  Busy();

 private:
  bool mArray[UARCH_NUM_PHYSICAL_REG];
  ULONG dNumRead; 
  ULONG dNumSet; 
  ULONG dNumClear; 
};

#endif
