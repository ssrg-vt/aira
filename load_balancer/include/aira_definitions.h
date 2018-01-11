/*
 * AIRA definitions used by the client and server.
 *
 * Author: Rob Lyerly
 * Date: 7/8/2015
 */

#include <stdint.h>

#ifndef _AIRA_DEFINITIONS
#define _AIRA_DEFINITIONS

///////////////////////////////////////////////////////////////////////////////
// Features
///////////////////////////////////////////////////////////////////////////////

/* Names to access individual features in the feature vector */
enum features {
	num_kernels = 0,
	num_mem_transfers,
	total_memtransfersize,
	avg_num_threads,
	num_instr,
	arith_instr,
	branches,
	divergent_branches,
	atomic_instr,
	global_mem_fence,
	global_load,
	global_store,
	local_load,
	local_store,
	gld_coalescing,
	gst_coalescing,
	NUM_FEATURES // Not a real feature!
};

/* Feature vector */
struct kernel_features {
	int kernel;
	double feature[NUM_FEATURES];
};

///////////////////////////////////////////////////////////////////////////////
// Resource allocation
///////////////////////////////////////////////////////////////////////////////

/*
 * Resource allocation for the applications.	In the spirit of OpenCL,
 * specifies a platform, device and the number of compute units to use (if
 * applicable).
 */
struct resource_alloc {
	uint8_t platform;
	uint8_t device;
	uint16_t compute_units;
};

#endif /* _AIRA_DEFINITIONS */

