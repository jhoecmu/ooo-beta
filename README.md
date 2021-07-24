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

--------------

Type "make" to build the executable "ooo".  

An execution of "ooo" simulates the instruction pipeline's
cycle-by-cycle RTL events.  To become familiar with this model
initially, execute "ooo" in a good debugger so you can step through
the RTL modeled events.  You can control error checking and debugging
print by setting DEBUG_LEVEL in sim.h.

Continue reading datapath.h, arch.h, uarch.h, and trace.h.

Todo:
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
