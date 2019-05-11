#ifndef DATAPATH_H
#define DATAPATH_H
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

#include "fetch.h"

/*
 * One invocation of datapath() corresponds to 1 cycle of the
 * out-of-order core. I_ arguments correspond to input ports.  O_
 * pass-by-pointer arguments correspond to output ports and hold the
 * final combinational values when datapath() returns.  O_ arguements
 * are assigned to and never read from inside the function.  
 *
 * Static variables and objects inside datapth() represent synchronous
 * states that are carried from invocation to invocation (i.e.,
 * cycle-to-cycle).  They are only modified at the end of the function.
 *
 * Please refer to [Zhao and Hoe, "Using Vivado-HLS for Structural
 * Design: a NoC Case Study" 2017. arXiv:1710.10290] for a full
 * discussion of discipline on using C++ to model RTL.  Further, this
 * RTL model is compatible with Xilinx Vivado HLS synthesis.
 */

void datapath(bool I_Reset, // Must be asserted in the first-call to
			    // datapath
	      FetchBundle I_2FetchedInsts, // Instructions and associated
				       // info presented for decode
				       // this cycle
	      ULONG *O_2Accept, // Number of instructions accepted by
				// the datapath this cycle
	      bool *O_6Rewind,  // Requesting a misprediction redirect
	      bool *O_0Restart, // Requesting a on-exception redirect
	      ULONG *O_0GotoPC  // Redirect address
	      ); 

#endif
