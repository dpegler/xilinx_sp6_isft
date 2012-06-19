CAUTION: 
This Tactical Update is intended as a fast response for the following
customer issue only. The risk inherent in this fast response is that there
is not enough time or resources available for the full regression testing
that is done for the Service Pack Updates and there is a higher risk of
introducing new problems. It is recommended that customers always install
the latest Service Pack Update, but only install Tactical Updates when needed
to resolve specific issues. This Tactical Update may not be compatible with
other Tactical Updates made available by Xilinx.

DESCRIPTION:
Xilinx Answer #34307
http://www.xilinx.com/support/answers/34307.htm

11.4 EDK, MPMC v5.04.a, Spartan-6 ES Silicon -  MCB memory access failure patch 
12.1 EDK, MPMC v6.00.a, Spartan-6 ES Silicon -  MCB memory access failure patch 
12.2 EDK, MPMC v6.01.a, Spartan-6 ES Silicon -  MCB memory access failure patch 

When using CES Spartan-6 devices, the MPMC memory access are unreliable. How do I resolve this issue?


INSTALLATION:
Extract this zip to the project pcore/ directory.

COMPATIBILITY:
This issue does not affect production devices, and this pcore must be removed when targeting production silicon. This version targets 11.4 and 12.1, the same changes will need to be made to all future MPMC designs using ES devices only.
