#ifndef REGFILE_H
#define REGFILE_H
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

#if (UARCH_ROB_RENAME)
#define MAX_REGFILE_READ (UARCH_DECODE_WIDTH*2+UARCH_RETIRE_WIDTH)
#define MAX_REGFILE_WRITE (UARCH_EXECUTE_WIDTH+UARCH_RETIRE_WIDTH)
#else
#define MAX_REGFILE_READ (UARCH_EXECUTE_WIDTH*2)
#define MAX_REGFILE_WRITE (UARCH_EXECUTE_WIDTH)
#endif

class RegFile {
 public:
  DataValue q5Read(PhysicalRegIdx preg);

  void a6Write(PhysicalRegIdx preg, DataValue val);

  void rReset();

  void simTick();

  // Constructor
  RegFile();

 private:
  DataValue mArray[UARCH_NUM_PHYSICAL_REG];
  ULONG dNumRead;
  ULONG dNumWrite;
};

#endif
