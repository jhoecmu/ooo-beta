#ifndef SIM_H
#define SIM_H
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

//----------------------------------------------------
// Catch-all baseline SIM defines, declarations and utilities
//----------------------------------------------------

#include <cassert>
#include <iostream>
#include <cstdlib>

#define DEBUG_VERBOSE (4) /* verbose state dump */
#define DEBUG_FULL (3)    /* with assert and print trace */
#define DEBUG_SILENT (2)  /* with assert, no print */
#define DEBUG_TRACE (1)   /* no asserts, print trace */
#define DEBUG_NONE (0)    /* no asserts, no print */
//#define DEBUG_LEVEL DEBUG_VERBOSE
#define DEBUG_LEVEL DEBUG_FULL
//#define DEBUG_LEVEL DEBUG_SILENT
//#define DEBUG_LEVEL DEBUG_NONE

//#define DEBUG_PRINT_DOWNSAMPLE (1<<12)
#define DEBUG_PRINT_DOWNSAMPLE (1)

using namespace std;

//
// use asserts liberally; they are easy to turn off later when you are sure
//
#if (DEBUG_LEVEL>=DEBUG_SILENT)
#define USAGEWARN(a,b) { if (!(a)) { cerr << "!!!USAGE ASSERTION FAILED!!! "<< (b); assert((a)); } }
#define ASSERT(a) (assert((a))) 
#else
#define USAGEWARN(a,b) 
#define ASSERT(a)
#endif

//
// typedef's to make changing HW data sizes more transparent
//
typedef unsigned long long ULONGLONG;
typedef long long LONGLONG;

typedef unsigned long ULONG;
typedef signed long LONG;

typedef unsigned int UINT;
typedef signed int INT;

typedef unsigned short USHORT;
typedef signed short SHORT;

static inline void simCheckTypes() {
  USAGEWARN((sizeof(ULONG)==8), "LONG is not 8 bytes.\n");
  USAGEWARN((sizeof(UINT)==4), "INT is not 4 bytes.\n");
  USAGEWARN((sizeof(ULONGLONG)==8), "LONGLONG is not 8 bytes.\n");
  USAGEWARN((sizeof(SHORT)==2), "SHORT is not 2 bytes.\n");
  USAGEWARN((sizeof(void*)==8), "pointer is not 8 bytes.\n");
  USAGEWARN((sizeof(void*)==sizeof(ULONGLONG)), "pointer is not same size as LONGLONG.\n");
}

//
// simulation global variables and helper functions
//

typedef LONGLONG Serial;
typedef LONGLONG Tick;

extern volatile Tick simTimer;  // global time tick
extern volatile bool simTock;  // global clock phase

static const Tick TICK_CYC=50;
static const double TIME_SCALE=1e-10;  

static const Tick TICK_INF=((Tick)(((0x7fffffffffffffff)/TICK_CYC)*TICK_CYC));
static const Tick TICK_INIT=((Tick)(0xffffffffffffffff));
static const Tick TICK_ZERO=((Tick)(0x0));

static inline Tick earlierOf(Tick a, Tick b) {
  return (a<=b)?a:b;
} 
static inline bool isEarlier(Tick a, Tick b) {
  return (a<b);
}
static inline bool isSameOrEarlier(Tick a, Tick b) {
  return (a<=b);
}

static inline Tick laterOf(Tick a, Tick b) {
  return (a>=b)?a:b;
} 
static inline bool isLater(Tick a, Tick b) {
  return (a>b);
}
static inline bool isSameOrLater(Tick a, Tick b) {
  return (a>=b);
}

static inline bool isTwoPower(ULONG val) {
  return ((val&(val-1))==0);
}

#define MIN(a,b) (((a)<=(b))?(a):(b)) 

#define MAX(a,b) (((a)>=(b))?(a):(b)) 

static inline ULONG popCount(ULONG val) {
  ULONG count=0;
  for(ULONG i=0;i<(8*sizeof(ULONG));i++) {
    count+=(val&1)?1:0;
    val=val>>1;
  }
  return count;
}
 
#endif

