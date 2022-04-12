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

This project contains an executable RTL model of a superscalar
speculative out-of-order instruction pipeline modeled after MIPS R10K
(as described in [Yeager, "The MIPS R10000 superscalar
microprocessor," 1996. https://ieeexplore.ieee.org/document/491460]).
Besides presenting an elegant clockwork of a design, the Yeager paper
is exceptional in that it actually says enough to enable one to work
out the key details.

Departing from pure R10K, this model also support the option to
perform ROB register renaming instead of the "physical file" approach.
(Controlled by UARCH_ROB_RENAME in uarch.h.)  If using ROB rename,
there is further the option to maintain a Metaflow DRIS-like
centralized bookkeepping structure to passively check rename and issue
correctness.

The executable RTL model is expressed in C++ following the discipline
prescribed in [Zhao and Hoe, "Using Vivado-HLS for Structural Design:
a NoC Case Study" 2017. arXiv:1710.10290].  By writing this in C++,
the goal is to make it understandable to more people and easier to
play with.  The model is true RTL.

***This model is for instructional illustration of an out-of-order
core.*** This model is fully detailed but it is not a complete
processor implementation. The model currently is only concerned with 2
opcodes ADD and BEQ to exercise dataflow instruction execution,
1-cycle fast rewind of wrongpath speculative execution, and precise
exception.  The model currently covers only register dataflow. LW and
SW need to be added in future work.  The model also does not model
instruction-fetch front-end (which is well decoupled in concept and in
practice from the dataflow execution portion of the pipeline).  Please
see [Hoe, "Superscalar Out-of-Order Demystified in Four Instructions,"
2003.  http://users.ece.cmu.edu/~jhoe/distribution/2003/wcae03.pdf.]
for the motivation behind this approach to understanding superscalar
out-of-order datapath.

Despite the above, there is no easy way to casually engage the study
superscalar out-of-order processor design.  With all the details,
the model is complicated because the subject is complicated.
This model is most useful if you have already read the Yeager paper
and want to see the details in exactitude and in action.  A textbook
level understanding of Tomasulo is not a good starting point.
Keep in mind, R10K is more than 20 years old. Today, this model is, 
at best, a baseline for you to start to understand the ensuing advances 
in the interest of higher performance and energy efficiency.

***The model is heavily instrumented with consistency/invariant
checking assertions. The degree of paranoia is controlled by the
debugging level set in sim.h.  When checking is enabled, instruction
entities flowing in the datapath carries a magic cookie of expected
operand value and instruction outcome precomputed by the trace
generator.

Continue reading datapath.h, arch.h, uarch.h, and trace.h to understand
the code more

--------------

To get started:

Type "make" to build the executable "ooo".  

An execution of "ooo" simulates the instruction pipeline's
cycle-by-cycle RTL events.  To become familiar with this model
initially, execute "ooo" in a good debugger so you can step through
the RTL modeled events.  You can control error checking and debugging
print by setting DEBUG_LEVEL in sim.h.

As unpacked, the datapath is configured to be R10K like (see uarch.h).
The executable built is configured to execute a 100,000-long randomly generated 
instruction trace.  The trace is configured in trace.h.  The screen output should match
reference1. You can test that by "make regress1".

You can set #define UARCH_ROB_RENAME (1) in uarch.h to configure the
datapath to use the ROB (instead of R10K's physical register file) to
hold renamed register outputs of speculative instructions.  The screen output should match
reference2 ("make regress2").  Beyond this, you can experiment with 
customizing the datapath configuration in uarch.h.

To start, you may want to study the behavior of a simpler datapath. Try reducing the superscalar degree
in uarch.h.

#define UARCH_DECODE_WIDTH    (1)

#define UARCH_RETIRE_WIDTH    (1)

#define UARCH_EXECUTE_WIDTH   (1)

If you want to study the behavior of a specific instruction fragment, you
can set #define TRACE_RANDOM (0) in trace.h.  This will execute from the
instruction sequence in test.h.  Edit test.h to your liking.  (See test.h 
and arch.h for detail.)  You only have ADD and BEQ instructions.  Any 
instruction can be optionaly tagged to trap (forcing the pipeline to drain 
and restart).  BEQ needs to be pre-designated to resolve, when executed, 
as predicted correctly or incorrectly. Branch misprediction forces an 
immediate rewind and restart.  

An interesting small example with WAW is:

static Instruction test[]={

  {.opcode=ADD, .rd=R3, .rs1=R1, .rs2=R4},
  
  {.opcode=ADD, .rd=R2, .rs1=R1, .rs2=R3},
  
  {.opcode=ADD, .rd=R3, .rs1=R1, .rs2=R4},
  
  {.opcode=ADD, .rd=R4, .rs1=R3, .rs2=R4},
  
};

--------------

If you run the above simple test with the non-superscalar uarh suggested, you should
see the following screen output.

cyc1:D    :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

    <<  In cycle 1, instruction serial 0 (Add r3,r1,r4) is decoded.  td, ts1 and ts2 are the renamed physical register locations. 0000 is the branch rewind stack mask.>>
    <<  The number in parenthesis after the serial number is the depth of instruction on the wrong path. Any instruction with depth greater than 0 will eventually be invalidated and removed. This info is "magical" and not used by the datapath model.>>

cyc2: I   :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc2:D    :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

    << In cycle 2, instruction s1 is decoded; s0 is issued.>>  
    
cyc3:  O  :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc3: I   :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc3:D    :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

    << In cycle 3, instruction s2 is decoded; s1 is issued. s0 is fetching operand from RF in cyc 5.>>  

cyc4:  O  :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc4:   E :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc4: I   :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

cyc4:D    :s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

    << Self explanatory.  s0 is executing in cyc 5.>>  

cyc5:  O  :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

cyc5:    R:s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc5:   E :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc5: I   :s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

    << Self explanatory.  s0 is retiring in cyc 5.>> 


--------------
  

Going back to the default R10K-based wide uarch produces more interesting behaviors.  

cyc1:D    :s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

cyc1:D    :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

cyc1:D    :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc1:D    :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

    << All 4 insts decoded in cycle 1.>>  

cyc2: I   :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc2: I   :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

    << Only s0 and s2 are issued (operands ready)  

cyc3:  O  :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc3:  O  :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

cyc3: I   :s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

cyc3: I   :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

    << s3 and s1 are issued. Operands from s0 and s2 will be ready with forwarding.>>  

cyc4:  O  :s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

cyc4:  O  :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc4:   E :s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc4:   E :s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

    << s0 retires in cyc4. s2 is completed but cannot retire out of order. >>

cyc5:    R:s0(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t32 ts1=t1 ts2=t4 0000

cyc5:   E :s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

cyc5:   E :s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc6:    R:s1(0)ADD rd=R2 rs1=R1 rs2=R3 :: td=t33 ts1=t1 ts2=t32 0000

cyc6:    R:s2(0)ADD rd=R3 rs1=R1 rs2=R4 :: td=t34 ts1=t1 ts2=t4 0000

cyc6:    R:s3(0)ADD rd=R4 rs1=R3 rs2=R4 :: td=t35 ts1=t34 ts2=t4 0000

    << superscalar retire of s1, s2 and s3 in order.>>
  
  
Setting #define DEBUG_LEVEL DEBUG_VERBOSE in sim.h will also dump out the contents of the instruction queue (reservation stations) and active list (ROB) cycle-by-cycle.
  
--------------

Future work:
* Need to finish comments (so far only datapath.cpp is "finished")
* Try out Vivado HLS
* (Beyond R10K) Add retirement map table option for faster exception restart
* (Beyond R10K) Add support for multiple branch speculation per cycle (to share a common single checkpoint restart)
* Add ld/sw
* Add fetch front-end 
* (Beyond R10K) SMT should be fun
* (Beyond R10K) Alpha clustered scheduling/execution 

--------------

Visit https://users.ece.cmu.edu/~jhoe/doku/doku.php?id=a_term_project_for_teaching_superscalar_out-of-order to find additional information.
