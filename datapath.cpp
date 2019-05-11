#define DATAPATH_CPP
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
#include "print.h"

#include "datapath.h"

//
// Datapath Objects
//
#include "activelist.h"
#include "alu.h"
#include "busy.h"
#include "exception.h"
#include "instq.h"
#include "regfile.h"
#include "rmap.h"
#include "checkpoint.h"

/*
 * datapath() models the superscalar speculative out-of-order
 * instruction pipeline modeled after MIPS R10K (as described in
 * [Yeager, "The MIPS R10000 superscalar microprocessor,"
 * 1996. https://ieeexplore.ieee.org/document/491460].)  The datapath
 * can also model ROB register renaming as an alternative to
 * physical file renaming in R10K.
 *
 * The modeled datapath has 7 stages
 * 
 * 1 Fetch: external to ooo datapath 
 * 2 Map: instructions register-renamed and entered into 
 *        the activelist (aka. ROB)
 * 3 Dispatch: mapped instructions are dispatched to instruction 
 *             queues (aka. reservation stations)
 * 4 Issue: readied instructions in instruction queues are scheduled 
 *          and issued
 * 5 Operand: operand fetched from register file (or captured forwarding)
 * 6 Execute: instruction executed (branch resolved, exception 
 *            detected) and results written back
 * 7 Retire: oldest completed instructions in activelist retire in 
 *           program order
 *
 * *** stages 4 and 5 can be cascaded combinationally, as done in R10K, by
 * *** setting UARCH_CASCADE_ISSUE4_OPRND5 in uarch.h
 * 
 * There is a nominal Stage 0 for global signals such as exception handling.
 *
 * The datapath supports a parameterized decode width, execution width
 * and retirement width.  Instructions are renamed and scheduled for
 * execution in microdataflow order.  The datapath supports a
 * parameterized number of unresolved branch speculations; restarting
 * after a misprediction takes only 1 cycle using a rewind stack
 * technique.  On an exception, fetch is stopped and the pipeline
 * begins to drain.  Rewinding of out-of-order state back to the
 * precise exception point is a sequential process over a variable
 * number of cycles. (Single cycle if with ROB rename.)
 *
 * Currently, this datapath only models the execution of ALU and branch
 * instructions.
 *
 * One invocation of datapath() corresponds to 1 cycle of the entire
 * out-of-order datapath.  The first part of the code generates the
 * combinational signals. In this first part, one query methods
 * (corredponding to combination logic) of datapath objects can be
 * invoked.  In the second part, synchronous state changes are
 * committed by either (1) writing to the static variables
 * indatapath() serving as pipeline registers or (2) invoking the
 * action methods of objects.
 *
 * Please refer to [Zhao and Hoe, "Using Vivado-HLS for Structural
 * Design: a NoC Case Study" 2017. arXiv:1710.10290] for a full
 * discussion of discipline on using C++ to model RTL.  Further, this
 * RTL model is compatible with Xilinx Vivado HLS synthesis.
 */

void datapath(bool I_Reset, // Must be asserted in the first-call to
			    // datapath
	      FetchBundle I_2FetchedInsts, // Instructions and
				           // associated info
				           // presented for decode
				           // this cycle
	      ULONG *O_2Accept, // Number of instructions accepted by
				// the datapath this cycle
	      bool *O_6Rewind,  // Requesting a misprediction redirect
	      bool *O_0Restart, // Requesting a on-exception redirect
	      ULONG *O_0GotoPC  // Redirect address
	      ) {

  //
  // instantiate static datapath objects containing state
  //
  static ActiveList activelist;             // in-order buffer for
					    // inflight instructions
  static Alu alu[UARCH_EXECUTE_WIDTH];      // this is superscalar!!
  static Busy busy;                         // busy bit table
  static Checkpoint checkpoint;             // checkpoint management
  static Exception exception;               // exception tracking unit
  static InstQ instq[UARCH_EXECUTE_WIDTH];  // ooo scheduler, aka
					    // reservation station
  static RegFile rf;                        // register file, arch+rename
  static RMapSS rmap;                       // register map table

  //
  // Instantiate pipeline registers (static variables persistent
  // across calls to datapath()).  The position of the pipeline
  // register is indicated by the suffix
  //
  static bool handleException_0L0;       // handleException_0 delayed by 1 cycle
  static ULONG redirectPC_0L0;           // redirect PC for branch or
					 // exception restart
  static FetchBundle fetchBndl_2L3;      // fetchBndl_2 delayed by 1 cycle into stage 3 
  static RMapBundle renamedBndl_2L3;     // renamed operation bundle to dispatch in stage 3
  static FreeRegBundle freeRegBndl_2L3;  // free reg used by inst to dispatch in stage 3 
  static ULONG numToDispatch_2L3;        // number of inst to dispatch in stage 3 
  static bool hasBR_2L3;                 // instBndl in stage 3 has a branch?

  static InstQEntry oprndFetchBndl_4L5[UARCH_EXECUTE_WIDTH];;  // execute bundle in stage 5 (operand)

  static InstQEntry executeBndl_5L6[UARCH_EXECUTE_WIDTH];;  // execute bundle in stage 6 (execute) 
  static DataValue vs1_5L6[UARCH_EXECUTE_WIDTH]; // vs1 for execute bundle in stage 6 (execute2)
  static DataValue vs2_5L6[UARCH_EXECUTE_WIDTH]; // vs2 for execute bundle in stage 6 (execute2)

  //
  // output port combinational "next" signal and their default values
  //
  ULONG OO_2Accept=0;
  bool OO_6Rewind=false;
  bool OO_0Restart=false;
  ULONG OO_0GotoPC=-1;

  if (I_Reset) {
    cout << "DEBUG_VERBOSE=" << DEBUG_VERBOSE << "\n";
    cout << "DEBUG_FULL=" << DEBUG_FULL << "\n";
    cout << "DEBUG_SILENT=" << DEBUG_SILENT << "\n";
    cout << "DEBUG_TRACE=" << DEBUG_TRACE << "\n";
    cout << "DEBUG_NONE=" << DEBUG_NONE << "\n";
    cout << "DEBUG_LEVEL=" << DEBUG_LEVEL << "\n";
    cout << "DEBUG_PRINT_DOWNSAMPLE=" << DEBUG_PRINT_DOWNSAMPLE << "\n";

    cout << "ARCH_NUM_LOGICAL_REG=" << ARCH_NUM_LOGICAL_REG << "\n";
    cout << "UARCH_ROB_RENAME=" << UARCH_ROB_RENAME << "\n";
    cout << "UARCH_CASCADE_ISSUE4_OPRND5=" << UARCH_CASCADE_ISSUE4_OPRND5 << "\n";
#if (UARCH_ROB_RENAME)
    cout << "UARCH_DRIS_CHECKER=" << UARCH_DRIS_CHECKER << "\n";
#endif
    cout << "UARCH_DECODE_WIDTH=" << UARCH_DECODE_WIDTH << "\n";
    cout << "UARCH_RETIRE_WIDTH=" << UARCH_RETIRE_WIDTH << "\n";
    cout << "UARCH_EXECUTE_WIDTH=" << UARCH_EXECUTE_WIDTH << "\n";
    cout << "UARCH_OOO_DEGREE=" << UARCH_OOO_DEGREE << "\n";
    cout << "UARCH_INSTQ_SIZE=" << UARCH_INSTQ_SIZE << "\n";
    cout << "UARCH_SPECULATE_DEPTH=" << UARCH_SPECULATE_DEPTH << "\n";

    cout << "UARCH_NUM_PHYSICAL_REG=" << UARCH_NUM_PHYSICAL_REG << "\n";
    
    // reset datapath ojects
    activelist.rReset();
    FOR_EXECUTE_WIDTH_i { alu[i].rReset(); }
    busy.rReset();
    checkpoint.rReset();
    exception.rReset();
    FOR_EXECUTE_WIDTH_i { instq[i].rReset(); }
    rf.rReset();
    rmap.rReset();

    // reset pipeline registers
    handleException_0L0=false;
    numToDispatch_2L3=0;
    hasBR_2L3=0;
    FOR_EXECUTE_WIDTH_i { oprndFetchBndl_4L5[i]=invalidEntryInstQ; }
    FOR_EXECUTE_WIDTH_i { executeBndl_5L6[i]=invalidEntryInstQ; }
  } else { 
    //
    // if not in reset; this is the main body of datapath()
    //

    //
    // Combinational signals that do not persist across invocations.
    // The stage of the signal is indicated by the suffix
    //
    bool exceptionPending_0;   // An exception has been raised but not
			       // yet oldest
    bool handleException_0;    // An exception instruction is oldest
			       // in activelist
    ULONG redirectPC_0;
#if (!UARCH_ROB_RENAME)
    UnmapBundle unmapBndl_0; // Read back of logged old "rd"
			     // mappings to walk register rename to
			     // back to exception point
    SpeculateMask groundMask_0;   // oldest checkpoint at the moment
#endif
    SpeculateMask dependOnMask_0; // bitmask of dependencies on not
				  // yet resolved branches

    FetchBundle fetchBndl_2;      // upto DECODE_WIDTH no. of instructions provided as input to datapath
    RMapBundle renamedBndl_2;     // upto DECODE_WIDTH no. of renamed instruction objects
    FreeRegBundle freeRegBndl_2;  // upto DECODE_WDITH no. of free registers available for rd remapping

    ULONG numToRename_2;          // no. of instruction accepted this cycle

    ULONG instqFree_2[UARCH_EXECUTE_WIDTH];  // no. of free slots in each instruction queue 
    LONG instqFreeTotal_2=0;                 // total number of slots in instruction queues
    bool hasBR_2;                           // current fetch bundle contains a branch instruction 
    ULONG newCheckPoint_2;                  // new checkpoint to use by the branch instruction

    bool ts1Busy_3[UARCH_DECODE_WIDTH];     // are rs1 operands of dispatching instructions pending?
    bool ts2Busy_3[UARCH_DECODE_WIDTH];     // are rs2 operands of dispatching instructions pending?

    InstQEntry issueBndl_4[UARCH_EXECUTE_WIDTH]; // instruction scheduled by instruction queue to issue

    DataValue vs1_5[UARCH_EXECUTE_WIDTH];   // rs1 operand value of executing instruction in stage 5
    DataValue vs2_5[UARCH_EXECUTE_WIDTH];   // rs2 operand value of executing instructions in stage 5

    AluOut aluOut_6[UARCH_EXECUTE_WIDTH];    // result value of executed instructions in stage 6
  
    bool hasException_6[UARCH_EXECUTE_WIDTH];  // execution resulted in an exception
    FOR_EXECUTE_WIDTH_i { hasException_6[i]=false; }

    SpeculateMask exceptionDependOn_6[UARCH_EXECUTE_WIDTH]; // speculation mask of exception instruction

    Cookie exceptionCookie_6[UARCH_EXECUTE_WIDTH]; // for debug: exception inst's magic cookie


    SpeculateMask rewindMask_6; // indicates which speculative branch
				// is rewinding; all instructions and
				// state depending on this
				// mispredicted branch need to be
				// flushed out
    FOR_SPECULATE_DEPTH_i { rewindMask_6.bit[i]=false; }

    SpeculateMask freeMask_6;  // indicates which speculative branch
			       // is confirmed; its slot in the branch
			       // rewind stack is freed and reclaimed
			       // for use by another speculation level
    FOR_SPECULATE_DEPTH_i { freeMask_6.bit[i]=false; }

    Cookie branchCookie_6; // for debug: resolving branch's magic cookie   

    RetireBundle retireBndl_7;

    { 
      //
      // for debug: prep debug state at the start of each "cycle"
      //
      activelist.simTick();
      FOR_EXECUTE_WIDTH_i { alu[i].simTick(); }
      busy.simTick();
      exception.simTick();
      FOR_EXECUTE_WIDTH_i { instq[i].simTick(); }
      rf.simTick();
      rmap.simTick();
      checkpoint.simTick();
    }

    { 
      //
      // Tick: this is the combinatinal signal generation phase.  No
      // state is modified.
      //
      simTock=0; // object's action methods are prevented
      
      { 
	//
	// Stage 0 handles global actions, most importantly exception restart
	//
	exceptionPending_0=exception.q0Pending();  // any pending exception (possibly speculative)?
	handleException_0=activelist.q0HandleException(); // pending exception oldest in ROB?

#if (!UARCH_ROB_RENAME)
	groundMask_0=checkpoint.q0Ground(); // if exception restart, oldest stack to skip to
	unmapBndl_0=activelist.q0Unmap();   // if exception restart, reg rmaps to undo by playback
#endif
	dependOnMask_0=checkpoint.q0GetMask(); // branch mask of currently unresolved branches

	if (handleException_0) {
	  ASSERT(exceptionPending_0);
	} 

	if (handleException_0) {
	  // only important in the last cycle of the rewind
	  redirectPC_0=activelist.q0GetExceptionPC();
	} else {
	  // look-up in case of a branch rewind ALU0 generates a
	  // branch rewind.  This is safe to do by default without
	  // decoding because only ALU0 handles branch (ala R10K)
	  if (executeBndl_5L6[0].valid) {
	    redirectPC_0=activelist.q0GetPC(executeBndl_5L6[0].atag);
	  }
	}
	
	if (handleException_0L0) { 
	  // for exception restart; needed in the last cycle
	  OO_0GotoPC=redirectPC_0L0;
	} else {
	  // for branch rewind
	  OO_0GotoPC=redirectPC_0;
	}
      } // Stage 0 

      { 
	//
	// Stage 2 (Map) decides how many instructions can be renamed
	// this cycle and lookup register renaming.
	//

	{ 
	  //
	  // howmany instruction to decode this cycle?
	  //
	  numToRename_2=UARCH_DECODE_WIDTH;

	  fetchBndl_2=I_2FetchedInsts;  // num fetch insts
	  numToRename_2=MIN(numToRename_2, fetchBndl_2.howmany);
	  freeRegBndl_2=activelist.q2GetFreeReg(); // num ROB entries and rename reg free
	  numToRename_2=MIN(numToRename_2, freeRegBndl_2.howmany);
	  
	  // num instq entries free 
	  FOR_EXECUTE_WIDTH_i {
	    instqFree_2[i]=instq[i].q2NumSlots();
	    instqFreeTotal_2+=instqFree_2[i];
	  }
	  instqFreeTotal_2-=numToDispatch_2L3;
	  instqFreeTotal_2=(instqFreeTotal_2>=0)?instqFreeTotal_2:0;
	  numToRename_2=MIN(numToRename_2,(ULONG)instqFreeTotal_2);

	  {
	    ULONG i;
	    hasBR_2=false;
	    for(i=0; i<numToRename_2; i++) {
	      // scan for branch instruction in fetch bundle.
	      // Following r10K, this datapath handles only 1 branch
	      // per cycle as the last instruction this cycle. This is
	      // due to (1) only ALU0 can handle branch and (2) how
	      // the branch stack works.
	      if (fetchBndl_2.inst[i].opcode==BEQ) {
		numToRename_2=i+1;
		hasBR_2=true;
		break;
	      }
	    }
	  }
	
	  if (hasBR_2) {
	    // must have free slot in ALU0; must have free rewind stack slot
	    if ( (instqFree_2[0]<(ULONG)((hasBR_2?1:0)+(hasBR_2L3?1:0))) ||
		 (!checkpoint.q2HasFree())) {
	      // if not, stop decode this cycle before branch
	      hasBR_2=false;
	      ASSERT(numToRename_2);
	      numToRename_2--;
	    } 
	  }
	}

	if (hasBR_2) {
	  // rewind stack slot to use for this cycle's branch
	  newCheckPoint_2=checkpoint.q2NextFree();
	}
	
	//
	// **RENAME** up to numToRename_2 number of instructions
	//
	renamedBndl_2=rmap.q2GetMapSS(numToRename_2, fetchBndl_2.inst, freeRegBndl_2.free);

	{ // fill out remaining renamedBndl_2.op fields
	  FOR_DECODE_WIDTH_i {
	    // okay to overrun numToRename_2; don't care for rest
	    renamedBndl_2.op[i].opcode=fetchBndl_2.inst[i].opcode;
	    renamedBndl_2.op[i].predTaken=fetchBndl_2.predTaken[i];
	    renamedBndl_2.op[i].oparity=fetchBndl_2.oparity[i];
	  }

	  if (hasBR_2) { // could be forall UARCH_DECODE_WIDTH
	    ASSERT(numToRename_2);
	    ASSERT(fetchBndl_2.inst[numToRename_2-1].opcode==BEQ);
	  }

	  FOR_DECODE_WIDTH_i {
	    // only important that the branch gets it; don't care for rest
	    renamedBndl_2.op[i].checkpoint=newCheckPoint_2;
	  }

	  {
	    FOR_DECODE_WIDTH_i {
	      // okay to overrun numToRename_2; don't care for rest
	      renamedBndl_2.op[i].dependOn=dependOnMask_0;
	    }
	  }
	}
      } // Stage 2 Map

      { 
	//
	// Stage 3 Dispatch: look up busy status of operands.  This
	// instructions will be dispatched to the instq this cycle.
	//
	FOR_DECODE_WIDTH_i {
	  // okay to overrun numToRename_2; don't care for rest
	  ts1Busy_3[i]=busy.q3IsBusy(tagToPRegIdx(renamedBndl_2L3.op[i].ts1));
	  ts2Busy_3[i]=busy.q3IsBusy(tagToPRegIdx(renamedBndl_2L3.op[i].ts2));
	}
      }
    
      { 
	//
	// Stage 4 Issue: ask each instq for 1 readied instruction
	//
	FOR_EXECUTE_WIDTH_i { issueBndl_4[i]=instq[i].q4Readied(); }
      }
    
#if (UARCH_CASCADE_ISSUE4_OPRND5)
      // R10K select instruction for issue and fetch its operands in
      // same cycle.  In this mode, oprndFetchBndl_4L5 receives
      // issueBndl_4 combinationally instead of next cycle (stage 5)
      FOR_EXECUTE_WIDTH_i { oprndFetchBndl_4L5[i]=issueBndl_4[i]; }
#endif

      { 
	//
	// Stage 5 Operand Fetch (subject to forwarding further
	// down). Not much else going on this cycle.
	//
	FOR_EXECUTE_WIDTH_i {
	  if (oprndFetchBndl_4L5[i].valid) {
	    prettyPrint(OSTAGE, oprndFetchBndl_4L5[i].op, oprndFetchBndl_4L5[i].cookie); 
	  }
	  vs1_5[i]=rf.q5Read(tagToPRegIdx(oprndFetchBndl_4L5[i].op.ts1));
	  vs2_5[i]=rf.q5Read(tagToPRegIdx(oprndFetchBndl_4L5[i].op.ts2));
	}
      } // Stage 5 Operand
      
      { 
	// 
	// Stage 6 (Execute) excutes the instructions
	// superscalarly. If ALU0 completes a Branch, appropriate
	// rewind or free mask is generated.  Exception is checked and
	// signaled.
	//
	FOR_EXECUTE_WIDTH_i {
	  aluOut_6[i]=alu[i].q6Execute((executeBndl_5L6[i].valid),  // valid op
				       executeBndl_5L6[i].op,       // opcode
				       vs1_5L6[i], vs2_5L6[i],      // operand
				       executeBndl_5L6[i].cookie);  // debug cookie
	  
	  if (aluOut_6[i].isBr) {
	    ASSERT(i==0); // only ALU0 can handle branch
	    branchCookie_6=executeBndl_5L6[i].cookie;

	    if (aluOut_6[i].isMispredict) {
	      ASSERT(executeBndl_5L6[i].cookie.inst.miss);
	      rewindMask_6.bit[executeBndl_5L6[i].op.checkpoint]=true;
	    } else {
	      ASSERT(!executeBndl_5L6[i].cookie.inst.miss);
	      freeMask_6.bit[executeBndl_5L6[i].op.checkpoint]=true;
	    }
	  }
	  
	  if (aluOut_6[i].isException) {
	    ASSERT(executeBndl_5L6[i].cookie.inst.exception);

	    hasException_6[i]=true; // flag exception
	    // info to pass to exception management unit
	    exceptionDependOn_6[i]=executeBndl_5L6[i].op.dependOn; // speculation status
	    exceptionCookie_6[i]=executeBndl_5L6[i].cookie;
	  }
	}
	
	{ // current datapath assumes at most 1 BR resolution per cycle
	  ULONG rewindCnt=0, freeCnt=0;
	  FOR_SPECULATE_DEPTH_i {
	    rewindCnt+=rewindMask_6.bit[i]?1:0;
	    freeCnt+=freeMask_6.bit[i]?1:0;
	  }
	  ASSERT((rewindCnt+freeCnt)<=1);
	}
      } // Stage 6 Execute

      { 
	//
	// Stage 7 Retire: ask activelist for instructions to retire
	// this cycle (in-order from oldest)
	//
	retireBndl_7=activelist.q7toRetire();
#if (UARCH_ROB_RENAME)
	// assertions to check consistency
	FOR_RETIRE_WIDTH_i {
	  RenameTag td=retireBndl_7.td[i];
	  DataValue val=rf.q5Read(tagToPRegIdx(td));
	  Cookie cookie=retireBndl_7.cookie[i];
	  retireBndl_7.val[i]=val;
	  if (i<retireBndl_7.howmany) {
	    ASSERT(tagEqual(td,cookie.op.td));
	    if (!tagEqual(td,ZeroRegTag)) {
	      ASSERT(retireBndl_7.val[i]==cookie.vd);
	    } else {
	      ASSERT(retireBndl_7.val[i]==0);
	    }
	  }
	}
#else
	// Not much happens at R10K retirement 
#endif
      }
    } // End of Tick

    //
    // combinational version of latched signals to capture forwarding
    //
    RMapBundle renamedBndl_3_;  
    renamedBndl_3_=renamedBndl_2L3;

    InstQEntry oprndFetchBndl_5_[UARCH_EXECUTE_WIDTH]; 
#if (!UARCH_CASCADE_ISSUE4_OPRND5)
    FOR_EXECUTE_WIDTH_i { oprndFetchBndl_5_[i]=oprndFetchBndl_4L5[i]; }
#endif

    InstQEntry executeBndl_6_[UARCH_EXECUTE_WIDTH];
    FOR_EXECUTE_WIDTH_i { executeBndl_6_[i]=executeBndl_5L6[i]; }
    
    { 
      //
      // Forwardings
      //
      FOR_EXECUTE_WIDTH_i {
	if (issueBndl_4[i].valid) {
	  // RAW depedencies are resolved when the dependent-on ALU
	  // instruction is issued. We need to forward to any
	  // dependent instructions in dispatch stage who received old
	  // status from the busy table.
	  FOR_DECODE_WIDTH_j {
	    // okay to overrun numToDispatch_2L3; don't care for rest
	    if (!tagEqual(issueBndl_4[i].op.td,ZeroRegTag)) {
	      if (tagEqual(issueBndl_4[i].op.td, renamedBndl_2L3.op[j].ts1)) {
		ASSERT(ts1Busy_3[j]);
		ts1Busy_3[j]=false;
	      }
	      if (tagEqual(issueBndl_4[i].op.td, renamedBndl_2L3.op[j].ts2)) {
		ASSERT(ts2Busy_3[j]);
		ts2Busy_3[j]=false;
	      }
	    }
	  }
	}
      }
      
      FOR_EXECUTE_WIDTH_i {
	if (executeBndl_5L6[i].valid) {
	  // Executed instructions in stage 6 forward results 1 stage
	  // back to stage 5 instructions that have just fetched
	  // operands from RF.  The dependent instructions were
	  // released for dataflow scheduling when the executed
	  // instructions was issued.
	  if (!tagEqual(executeBndl_5L6[i].op.td, ZeroRegTag)) {
	    FOR_EXECUTE_WIDTH_j {
	      // okay to forward to invalid slots
	      if (tagEqual(executeBndl_5L6[i].op.td, oprndFetchBndl_4L5[j].op.ts1)) {
		vs1_5[j]=aluOut_6[i].vd;
	      }
	      if (tagEqual(executeBndl_5L6[i].op.td, oprndFetchBndl_4L5[j].op.ts2)) {
		vs2_5[j]=aluOut_6[i].vd;
	      }
	    }
	  }
	}
      }

#if (UARCH_ROB_RENAME)
      for(ULONG i=0; i<retireBndl_7.howmany; i++) {
	RenameTag td=retireBndl_7.td[i];
	RenameTag ltag={.mapped=false, .idx=retireBndl_7.rd[i]};

	// On retirement, results of retiring instructions are copied
	// from ROB to RF.  Need to forward to inflight instructions
	// holding old renames (in 3 stages).  We could be clever and
	// avoid this by comparing the rename tags to the current
	// oldest slot in ROB to realize if a rename has expired.
	if (!tagEqual(td, ZeroRegTag)) {
	  FOR_DECODE_WIDTH_j {
	    if (tagEqual(td,renamedBndl_2.op[j].ts1)) {
	      renamedBndl_2.op[j].ts1=ltag;
	    }
	    if (tagEqual(td,renamedBndl_2.op[j].ts2)) {
	      renamedBndl_2.op[j].ts2=ltag;
	    }
	  }
	  FOR_DECODE_WIDTH_j {
	    if (tagEqual(td,renamedBndl_2L3.op[j].ts1)) {
	      renamedBndl_3_.op[j].ts1=ltag;
	    }
	    if (tagEqual(td,renamedBndl_2L3.op[j].ts2)) {
	      renamedBndl_3_.op[j].ts2=ltag;
	    }
	  }
	  FOR_EXECUTE_WIDTH_j {
	    if (tagEqual(td,issueBndl_4[j].op.ts1)) {
	      issueBndl_4[j].op.ts1=ltag;
	    }
	    if (tagEqual(td,issueBndl_4[j].op.ts2)) {
	      issueBndl_4[j].op.ts2=ltag;
	    }
	  }
	}
      }
#endif
      
      //
      // When branch resolves, whether confirm or rewind, a
      // corresponding mask is broadcasted.  Need to perform rewind or
      // confirm on inflight instructions and entities.
      //
      {
	FOR_EXECUTE_WIDTH_i {
	  if (hasException_6[i]) {
	    if (dependOnSpeculation(exceptionDependOn_6[i], freeMask_6)) {
	      // forward BR confirmation
	      exceptionDependOn_6[i].bit[whichSpeculation(freeMask_6)]=false;
	    }
	    if (dependOnSpeculation(exceptionDependOn_6[i], rewindMask_6)) {
	      // cancelled by BR mispredict
	      hasException_6[i]=false;
	    }
	  }

	  if (dependOnSpeculation(executeBndl_5L6[i].op.dependOn, rewindMask_6)) {
	    prettyPrint(EkSTAGE, executeBndl_5L6[i].op, executeBndl_5L6[i].cookie);
	    executeBndl_6_[i].valid=false;
	  }

	  if (dependOnSpeculation(issueBndl_4[i].op.dependOn, rewindMask_6)) {
	    prettyPrint(IkSTAGE, issueBndl_4[i].op, issueBndl_4[i].cookie);
	    issueBndl_4[i].valid=false;
	  }
	  if (dependOnSpeculation(issueBndl_4[i].op.dependOn, freeMask_6)) {
	    issueBndl_4[i].op.dependOn.bit[whichSpeculation(freeMask_6)]=false;
	  }

#if (!UARCH_CASCADE_ISSUE4_OPRND5)
	  if (dependOnSpeculation(oprndFetchBndl_4L5[i].op.dependOn, rewindMask_6)) {
	    prettyPrint(OkSTAGE, oprndFetchBndl_4L5[i].op, oprndFetchBndl_4L5[i].cookie);
	    oprndFetchBndl_5_[i].valid=false;
	  }
	  if (dependOnSpeculation(oprndFetchBndl_4L5[i].op.dependOn, freeMask_6)) {
	    oprndFetchBndl_5_[i].op.dependOn.bit[whichSpeculation(freeMask_6)]=false;
	  }
#else
	  // if stage 4 and 5 are collapsed, issueBndl and oprndFetchBndl are the same
	  FOR_EXECUTE_WIDTH_i { oprndFetchBndl_5_[i]=issueBndl_4[i]; }
#endif
	}

	FOR_DECODE_WIDTH_i {
	  if (dependOnSpeculation(renamedBndl_2.op[i].dependOn, freeMask_6)) {
	    renamedBndl_2.op[i].dependOn.bit[whichSpeculation(freeMask_6)]=false;
	  }
	}
	FOR_DECODE_WIDTH_i {
	  if (dependOnSpeculation(renamedBndl_2L3.op[i].dependOn, freeMask_6)) {
	    renamedBndl_3_.op[i].dependOn.bit[whichSpeculation(freeMask_6)]=false;
	  }
	}
      }
    } // Forwards 

    { 
      //
      // Tock: this state update phase analogous to "@(posedge clk)
      // Until now, everything corresponded to combinational signals.
      //
      simTock=1;  // object's query methods are prevented
    
      if (!(handleException_0 || handleException_0L0)) { 
	// Advancing state in stage 7 down to 2 when not waiting for
	// exception restart.
	//
	// If waiting for exception restart (i.e., oldest instruction
	// in ROB has exception), stage 2 through 7 does not advance
	// and will be reset before restart.

	{ 
	  //
	  // stage 7 Retire
	  //
	  activelist.a7Retire(retireBndl_7); // retire oldest completed, non-exception instructions

#if (UARCH_ROB_RENAME)
	  ASSERT(retireBndl_7.howmany<=UARCH_RETIRE_WIDTH);
	  for(ULONG i=0; i<retireBndl_7.howmany; i++) {
	    LogicalRegName rd=retireBndl_7.rd[i];
	    RenameTag td=retireBndl_7.td[i];
	    DataValue val=retireBndl_7.val[i];

	    ASSERT(rd==retireBndl_7.cookie[i].inst.rd);
	    ASSERT(tagEqual(td,retireBndl_7.cookie[i].op.td));
	    if (rd!=0) {
	      ASSERT(val==retireBndl_7.cookie[i].vd);
	    }

	    // if ROB rename, write retiring instruction's result to
	    // the in-order-commit register file
	    rf.a6Write((PhysicalRegIdx)rd, val);

	    // unmap tag from map table if the latest
	    rmap.a7Unmap(rd, td);

	    {
	      // unmap tag from operands of instructions in the instqs
	      RenameTag temp={.mapped=false, .idx=rd};
	      FOR_EXECUTE_WIDTH_j {
		instq[j].a7retireTag(td, temp, retireBndl_7.cookie[i]);
	      }
	    }
	  }
#endif
	} // stage 7 Retire
	
	{ // Stage 6 Execute
	  bool rewindedDEBUG=false;
	  bool clearedDEBUG=false;
	  
	  FOR_EXECUTE_WIDTH_i {
	    if (executeBndl_6_[i].valid) {
	      prettyPrint(ESTAGE, executeBndl_6_[i].op, executeBndl_6_[i].cookie);
	      
	      {
		// writeback to RF; skipped internally for non-ALU instructions (rd/td==0)
		rf.a6Write(tagToPRegIdx(executeBndl_6_[i].op.td), aluOut_6[i].vd);  
		
		// update completion status in activelist
		activelist.a6Complete(executeBndl_6_[i].atag);
		
		if (hasException_6[i]) {
		  // update exception status in activelist
		  activelist.a6Exception(executeBndl_6_[i].atag);
		}
	      }
	    }
	  }
	  
	  FOR_EXECUTE_WIDTH_i {
	    if (aluOut_6[i].isBr) {
	      // if a BR resolved in Execute
	      ASSERT(i==0); // only ALU0 can handle BR
	      
	      if (aluOut_6[i].isMispredict) {
		//
		// on a mispredicted branch, rewind state
		// (freeing the rewind stack in the process)
		//

		ASSERT(executeBndl_6_[i].cookie.inst.miss);
		ASSERT(!rewindedDEBUG);
		ASSERT(maskIsSetOnceSpeculation(rewindMask_6));
		rewindedDEBUG=true;
		
		// rewind to checkpointed state
#if (UARCH_ROB_RENAME)
		activelist.a6Rewind(executeBndl_6_[i].atag);
#else
		activelist.a6Rewind(executeBndl_6_[i].op.checkpoint);
#endif
		rmap.a6Rewind(executeBndl_6_[i].op.checkpoint);
		
		FOR_EXECUTE_WIDTH_j {
		  // squash inflight wrongpath instructions, if any
		  instq[j].a6Squash(rewindMask_6, branchCookie_6);
		}
		// cancel on-wrongpath pending exceptions, if any
		exception.a6Cancel(rewindMask_6, branchCookie_6);
		
		// rewind branch stack
		checkpoint.a6Rewind(rewindMask_6);
		
		// request a rewind on output
		OO_6Rewind=true;
	      } else {
		//
		// on a correctly predicted branch, free branch stack
		//
		ASSERT(!executeBndl_6_[i].cookie.inst.miss);
		ASSERT(!clearedDEBUG);
		ASSERT(maskIsSetOnceSpeculation(freeMask_6));
		clearedDEBUG=true;
		
		// clear depend-on bits of flight-masks so the
		// corresponding branch stack slot can be reused
		FOR_EXECUTE_WIDTH_j { 
		  instq[j].a6ClearMask(freeMask_6, branchCookie_6); 
		}
		exception.a6ClearMask(freeMask_6, branchCookie_6);
		
		// notice activelist and rmap do not need to be cleared
		// since they do not track speculation
		// dependencies. They work as directed by the checkpoint
		// objects.
		
		// free up this branch stack slot for reuse 
		checkpoint.a6Free(freeMask_6);
	      }
	    }
	  }
	  
	  FOR_EXECUTE_WIDTH_i {
	    if (hasException_6[i]) {
	      // raise new exceptions; if there is a pending exception
	      // already registered, the exception unit will work out
	      // the dominate relationship to keep the "less
	      // speculative" one. raising an exception stops fetching
	      // immediately to stop growing the state to be undone.
	      exception.a6Raise(exceptionDependOn_6[i], exceptionCookie_6[i]);
	    }
	  }
	  
	  // sanity check that either 0 or 1 one branch was executed
	  ASSERT(maskIsSetSpeculation(rewindMask_6)?
		 maskIsSetOnceSpeculation(rewindMask_6)&&rewindedDEBUG:
		 1);
	  ASSERT(maskIsSetSpeculation(freeMask_6)?
		 maskIsSetOnceSpeculation(freeMask_6)&&clearedDEBUG:
		 1);
	} // Stage 6 Execute
	
	// Stage 5 Operand Fetch
	FOR_EXECUTE_WIDTH_i {
	  if (oprndFetchBndl_5_[i].valid) {
	    // nothing here; see pipeline register shifting below.
	  }
	}
	
	// Stage 4 Issue
	FOR_EXECUTE_WIDTH_i {
	  if (issueBndl_4[i].valid) {
	    // issue scheduled instructions
	    instq[i].a4Issue(issueBndl_4[i].slotIdx);
#if (UARCH_DRIS_CHECKER)
	    // double check issue against centralized DRIS bookkeeping
	    activelist.d4CheckIssue(issueBndl_4[i]);
#endif
	    FOR_EXECUTE_WIDTH_j {
	      // release instq instructions dependent on this
	      // instruction for scheduling starting next cycle
	      instq[j].a4Release(issueBndl_4[i].op.td, issueBndl_4[i].cookie );
	    }
	    // clear busy table; inflight insts updated by forwarding above
	    busy.a4ClearBusy(tagToPRegIdx(issueBndl_4[i].op.td));
	    
	    // notice that the release and clear are happening before
	    // the producer instruction has actually started to execute.
	    //
	    // this is necessary to schedule a chain of dependent
	    // instruction.  scheduling is done on the basis that we
	    // know the consumer instruction will receive the operands
	    // by the time it executes.
	  }
	}
	
	{ // Stage 3 Dispatch
	  if (!(maskIsSetSpeculation(rewindMask_6))) {
	    // if rewinding (1-cycle), none of this happened.  Need to
	    // keep going on an exception though; until it is the
	    // oldest, it may be speculative.
	    ULONG inserted[UARCH_EXECUTE_WIDTH];
	    
	    FOR_EXECUTE_WIDTH_i { inserted[i]=0; }
	    
	    for(ULONG i=numToDispatch_2L3, j=0; i--; ) {
	      // Round-robin dispatch into instruction queue.  Notice,
	      // we are counting down, so if there is a BR (it must be
	      // the last of the bundle and there can only be one in a
	      // bundle), it is going into ALU0 without further fuss.
	      prettyPrint(DSTAGE, renamedBndl_3_.op[i], fetchBndl_2L3.cookie[i]);
	      
	      {
		ULONG loopcnt=0;
		while (inserted[j]==
		       (instqFree_2[j]-
			(((j==0)&&hasBR_2)?1:0))) {
		  // If decode this cycle (stage 2) is counting on
		  // having a slot next cycle in ALU0 for BR, we need
		  // to save the last one for that and use another
		  // instq instead.  If accounting was done correctly
		  // in stage 2 in the last cycle, there must be room.
		  j+=1;
		  j%=UARCH_EXECUTE_WIDTH;
		  loopcnt++;
		  ASSERT(loopcnt<=UARCH_EXECUTE_WIDTH);
		}
	      }
	      
	      if (renamedBndl_3_.op[i].opcode==BEQ) {
		ASSERT(hasBR_2L3); // BR is expected
		ASSERT(instqFree_2[0]>=(ULONG)((hasBR_2L3?1:0)+(hasBR_2?1:0))); // ALU0 has room 
		ASSERT(i==(numToDispatch_2L3-1)); // BR is last in bundle
		ASSERT(j==0); // dispatch into ALU0
	      }
	      
	      {
		// dispatching into instruction queue
		Operation op=renamedBndl_3_.op[i];
		instq[j].a3Insert(freeRegBndl_2L3.atag[i], op, 
				  ts1Busy_3[i], ts2Busy_3[i], 
				  fetchBndl_2L3.cookie[i]);
	      }
	      inserted[j]++;
	      j+=1;
	      j%=UARCH_EXECUTE_WIDTH;
	    }
	  }
	} // Stage 3 Dispatch
	
	{ // Stage 2 Map
	  if (!(exceptionPending_0||maskIsSetSpeculation(rewindMask_6))) {
	    // if exception pending or rewinding, none of this happened 
	    
	    OO_2Accept=numToRename_2;
	    
#if (DEBUG_LEVEL>=DEBUG_SILENT)
	    FOR_DECODE_WIDTH_i {
	      // okay to overrun numToRename_2; don't care for rest
	      fetchBndl_2.cookie[i].op=renamedBndl_2.op[i];
	    }      
#endif

	    // entire new instructions into activelist
	    activelist.a2Accept(numToRename_2, fetchBndl_2.inst, fetchBndl_2.pcLike,
#if (!UARCH_ROB_RENAME)
				renamedBndl_2.tdOld, 
#endif
#if (UARCH_DRIS_CHECKER)
				renamedBndl_2,
#endif
				fetchBndl_2.cookie);
	    
	    // set new rename mappings
	    rmap.a2SetMapSS(numToRename_2, fetchBndl_2.inst, freeRegBndl_2.free);
	    
	    FOR_DECODE_WIDTH_i {
	      // mark new dest registers busy
	      // okay to overrun numToRename_2; td is set to 0 by remap
	      ASSERT((i<numToRename_2)?1:tagEqual(renamedBndl_2.op[i].td,ZeroRegTag));
	      busy.a2SetBusy(tagToPRegIdx(renamedBndl_2.op[i].td));
	    }
	    
	    if (hasBR_2) {
	      // checkpoint into the branch rewind stack if BR this cycle
	      ASSERT(numToRename_2);
	      ASSERT(fetchBndl_2.inst[numToRename_2-1].opcode==BEQ);

	      checkpoint.a2New(newCheckPoint_2);
#if (!UARCH_ROB_RENAME)
	      activelist.a2CheckPoint(newCheckPoint_2);
#endif
	      rmap.a2CheckPoint(newCheckPoint_2);
	    }
	  }
	} // Stage 2 Map
      } // stage 7 down to 2
      
      { // Stage 0
	if (handleException_0) {
	  // oldest instruction is an exception so it is happening;
	  // prepare state for restart; may take multiple cycles in
	  // R10K to reconstruct the map table serially

	  FOR_EXECUTE_WIDTH_i { instq[i].rReset(); alu[i].rReset(); }

#if (UARCH_ROB_RENAME)
	  activelist.rReset();
	  rmap.rReset();
	  busy.rReset();
	  checkpoint.rReset();
#else
	  if (maskIsSetSpeculation(dependOnMask_0)) {
	    // first recover off the oldest entry on rewind stack if
	    // one is available
	    ASSERT(maskIsSetOnceSpeculation(groundMask_0));
	    checkpoint.a6Rewind(groundMask_0);
	    rmap.a6Rewind(whichSpeculation(groundMask_0));
	    activelist.a6Rewind(whichSpeculation(groundMask_0));
	  } else {
	    // then walk back the mappings sequentially
	    ASSERT(unmapBndl_0.howmany!=0);
	    rmap.a0UnmapSS(unmapBndl_0.howmany, unmapBndl_0.rd, unmapBndl_0.tdOld);
	    activelist.a0Unmap(unmapBndl_0.howmany);  // walk back the activelist youngest first
	  }
#endif
	}

	if (handleException_0L0&&(!handleException_0)) {
	  // last cycle of exception handling; ready to restart

	  OO_0Restart=true;
	  exception.a0ClearPending();

	  FOR_EXECUTE_WIDTH_i { instq[i].rReset(); alu[i].rReset(); }

#if (UARCH_ROB_RENAME)
	  activelist.rReset();
	  rmap.rReset();
	  busy.rReset();
	  checkpoint.rReset();
#else
	  // checkpoint.a0Continue();
#endif
	}
      } // Stage 0

      { 
	// 
	// Shifting of pipeline latches declared at the datapath level
	//
	FOR_EXECUTE_WIDTH_i {
	  if (handleException_0L0 || handleException_0) {
	    executeBndl_5L6[i].valid=false;
	  } else {
	    executeBndl_5L6[i]=oprndFetchBndl_5_[i];
	  }
	  
#if (UARCH_CASCADE_ISSUE4_OPRND5)
	  // the register for oprndFetchBndl_4L5[i] doesn't exist if
	  // issue and oprand stage are cascaded combinationally.
#else
	  if (handleException_0L0 || handleException_0) {
	    oprndFetchBndl_4L5[i].valid=false;
	  } else {
	    oprndFetchBndl_4L5[i]=issueBndl_4[i];
	  }
#endif
          vs1_5L6[i]=vs1_5[i];
          vs2_5L6[i]=vs2_5[i];
        }

	if (!(exceptionPending_0||maskIsSetSpeculation(rewindMask_6))) {
	  numToDispatch_2L3=numToRename_2;
	  freeRegBndl_2L3=freeRegBndl_2;
	  fetchBndl_2L3=fetchBndl_2;
	  renamedBndl_2L3=renamedBndl_2;
	  hasBR_2L3=hasBR_2;
	} else {
	  numToDispatch_2L3=0;
	  freeRegBndl_2L3.howmany=0;
	  fetchBndl_2L3.howmany=0;
	  renamedBndl_2L3.howmany=0;
	  hasBR_2L3=false;
	}
	
	handleException_0L0=handleException_0;
        redirectPC_0L0=redirectPC_0;
      } // Pipeline Latches
    } // Tock
  } // Not in reset

  // set output port signals
  *O_2Accept=OO_2Accept;
  *O_6Rewind=OO_6Rewind;
  *O_0Restart=OO_0Restart;
  *O_0GotoPC=OO_0GotoPC;
}



