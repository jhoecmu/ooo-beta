#ifndef UARCH_H
#define UARCH_H
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

//////////////////////////////////
//
// First-Order datapath parameters 
//
//////////////////////////////////

#define UARCH_USE_BASELINE (1)   // use baseline config below
#define UARCH_ROB_RENAME (0)     // ROB rename vs physical file
#define UARCH_CASCADE_ISSUE4_OPRND5 (0)  // collapse issue and operand fetch to match R10K 

#if (UARCH_ROB_RENAME)
#if (DEBUG_LEVEL>=DEBUG_SILENT)
#define UARCH_DRIS_CHECKER (1) // add Metaflow DRIS centralized
			       // bookeeping to activelist; provided
			       // passive checking on renaming and issue
#endif
#endif

//////////////////////////////////
//
// Second-Order datapath parameters
//
//////////////////////////////////

#if (!UARCH_USE_BASELINE)

// for hacking
#define UARCH_DECODE_WIDTH    (1)
#define UARCH_RETIRE_WIDTH    (1)
#define UARCH_EXECUTE_WIDTH   (1)
#define UARCH_OOO_DEGREE      (32)  // size of ROB
#define UARCH_INSTQ_SIZE      (16)  // size of each InstQ
#define UARCH_SPECULATE_DEPTH (4)   // depth of BR stack

#else

// for regression
#define UARCH_DECODE_WIDTH    (4)
#define UARCH_RETIRE_WIDTH    (4)
#define UARCH_EXECUTE_WIDTH   (3)
#define UARCH_OOO_DEGREE      (32)
#define UARCH_INSTQ_SIZE      (16)
#define UARCH_SPECULATE_DEPTH (4)

#endif

////////////////////////////////////
//
// uarch related types and functions
//
////////////////////////////////////


//
// Physical registers and related
// 
#define UARCH_NUM_PHYSICAL_REG (UARCH_OOO_DEGREE+ARCH_NUM_LOGICAL_REG)
typedef ULONG PhysicalRegIdx;  

typedef struct {
#if (UARCH_ROB_RENAME)
  bool mapped;
#endif
  ULONG idx;
} RenameTag;  // physical register index

static const RenameTag ZeroRegTag= { 
#if (UARCH_ROB_RENAME)
  .mapped=false, 
#endif
  .idx=0
};

static inline bool tagEqual(RenameTag a, RenameTag b) {
  bool result=(
#if (UARCH_ROB_RENAME)
	     (a.mapped==b.mapped)&&
#endif
	     (a.idx==b.idx)
	     );
  return result;
}

#if (UARCH_DRIS_CHECKER)
static inline bool drisTagIdxEqual(RenameTag a, RenameTag b) {
  bool result=(
	     (a.mapped==b.mapped)&&
	     ((a.idx%UARCH_OOO_DEGREE)==(b.idx%UARCH_OOO_DEGREE))
	     );
  return result;
}
#endif

static inline PhysicalRegIdx tagToPRegIdx(RenameTag a) {
#if (UARCH_ROB_RENAME)
  if (a.mapped) {
    ASSERT(a.idx<UARCH_OOO_DEGREE);
    return ARCH_NUM_LOGICAL_REG+a.idx;
  } 
  ASSERT(a.idx<ARCH_NUM_LOGICAL_REG);
#endif  
  ASSERT(a.idx<UARCH_NUM_PHYSICAL_REG);
  return a.idx;
}

//
// Branch Mask
// 
typedef struct {
  bool bit[UARCH_SPECULATE_DEPTH];
} SpeculateMask;


//
// Renamed instruction object in OOO datapath
//

typedef struct {
  OpCode opcode;
  RenameTag td, ts1, ts2; 
  bool predTaken;
  bool oparity;
  ULONG checkpoint;
  SpeculateMask dependOn;
} Operation;

//
// Loop shorthands (to help mimize typing and typo)
//

#define FOR_DECODE_WIDTH_i for(ULONG i=0;i<UARCH_DECODE_WIDTH;i++)
#define FOR_DECODE_WIDTH_j for(ULONG j=0;j<UARCH_DECODE_WIDTH;j++)

#define FOR_EXECUTE_WIDTH_i for(ULONG i=0;i<UARCH_EXECUTE_WIDTH;i++)
#define FOR_EXECUTE_WIDTH_j for(ULONG j=0;j<UARCH_EXECUTE_WIDTH;j++)

#define FOR_SPECULATE_DEPTH_i for(ULONG i=0;i<UARCH_SPECULATE_DEPTH;i++)
#define FOR_SPECULATE_DEPTH_j for(ULONG j=0;j<UARCH_SPECULATE_DEPTH;j++)

#define FOR_RETIRE_WIDTH_i for(ULONG i=0;i<UARCH_RETIRE_WIDTH;i++)
#define FOR_RETIRE_WIDTH_j for(ULONG j=0;j<UARCH_RETIRE_WIDTH;j++)

#define FOR_INSTQ_SIZE_i for(ULONG i=0;i<UARCH_INSTQ_SIZE;i++)

#endif
