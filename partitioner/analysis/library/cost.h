// cost.h -- Provide costs of doing things in Popcorn Linux.
//
// This file is distributed under the MIT license, see LICENSE.txt.

#ifndef _COST_H
#define _COST_H

#include "types.h"

// All times (migrationCost, pageFaultCost, xeonComputeCost) are all absolute
// numbers, in nanoseconds.  Bias, parallelism and slowdown numbers are
// relative numbers (i.e. they are used to scale absolute numbers).

// Bias controls how aggressively we attempt to partition a program.  It is
// implemented by increasing the compute cost of a node, this reduces the
// relative cost of a data transfer making partitioning more attractive.  A
// good number is somewhere between 1 and 5 (inclusive).
const int bias = 1;

// The cost to migrate a thread from kernel 0 to kernel 1 (or vice-versa).
const int migrationCost = 900*1000*4; // 900us per thread on host.

// The cost to bring in a missing page to kernel 1, from kernel 0's memory.
const int pageFaultCost = 50*1000; // 50us

// Performance numbers for the Xeon chip (E5-2609 @ 2.5GHz).
const int xeonComputeCost = 5; // ~5ns average compute per memory access.
const int xeonParallelism = 4; // 4-cores, no HT.
const cost_t xeonCost = {xeonComputeCost*bias, xeonParallelism};

// Performance numbers for the Xeon Phi board (3120A @ 1.1GHz).
const int phiSlowdown = 11; // Measured Phi as 11x slower on single-thread code.
const int phiComputeCost = xeonComputeCost * phiSlowdown;
const int phiParallelism = 57*4; // 57-cores, 4-way HT.
const cost_t phiCost = {phiComputeCost*bias, phiParallelism};

#endif // _COST_H
