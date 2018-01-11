/*
 * AMD.h
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#ifndef SRC_VENDOR_AMD_H_
#define SRC_VENDOR_AMD_H_

#include "MSR.h"

/*
 * AMD Model-specific registers & masks for reading power
 *
 * Taken from "BIOS and Kernel Developer's Guide (BKDG) for AMD Family 15h Models 00h-0Fh"
 * http://support.amd.com/TechDocs/42301_15h_Mod_00h-0Fh_BKDG.pdf
 */

/* MSR register addresses */
#define MSR_PROCESSOR_POWER_IN_TDP 0xc0010077
#define MSR_POWER_AVERAGING_PERIOD 0xc0010078

/* Offsets & masks */
// Processor Power in TDP values
#define MSR_BASE_TDP_OFFSET 0x10
#define MSR_BASE_TDP_MASK   0xffff
#define MSR_BASE_TDP( val ) GET_VAL(val, MSR_BASE_TDP_OFFSET, MSR_BASE_TDP_MASK)

// Power Averaging Period values
#define MSR_APM_TDP_LIMIT_OFFSET 0x20
#define MSR_APM_TDP_LIMIT_MASK   0x1fff
#define MSR_APM_TDP_LIMIT( val ) GET_VAL(val, MSR_APM_TDP_LIMIT_OFFSET, MSR_APM_TDP_LIMIT_MASK)

#define MSR_TDP_RUN_AVG_ACC_CAP_OFFSET 0x4
#define MSR_TDP_RUN_AVG_ACC_CAP_MASK   0x3fffff
#define MSR_TDP_RUN_AVG_ACC_CAP( val ) GET_VAL(val, MSR_TDP_RUN_AVG_ACC_CAP_OFFSET, MSR_TDP_RUN_AVG_ACC_CAP_MASK)

#define MSR_RUN_AVG_RANGE_OFFSET 0x0
#define MSR_RUN_AVG_RANGE_MASK   0xf
#define MSR_RUN_AVG_RANGE( val ) GET_VAL(val, MSR_RUN_AVG_RANGE_OFFSET, MSR_RUN_AVG_RANGE_MASK)

/* Calculations */
#define MSR_CURR_POWER_WATTS(ppit, pap) \
	((MSR_APM_TDP_LIMIT(pap) - \
	(MSR_TDP_RUN_AVG_ACC_CAP(pap) / (2 << (MSR_RUN_AVG_RANGE(pap)))) + \
	MSR_BASE_TDP(ppit)) / (2 << 16))

#endif /* SRC_VENDOR_AMD_H_ */
