/*
 * CPUID.h
 *
 *  Created on: May 13, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_UTILITY_CPUID_H_
#define SRC_UTILITY_CPUID_H_

#include <fcntl.h>

#include "common.h"

/* CPU-ID value extraction */
#define STEPPING_OFFSET 0x0
#define STEPPING_MASK   0xf
#define STEPPING( val ) GET_VAL(val, STEPPING_OFFSET, STEPPING_MASK)

#define RAW_MODEL_OFFSET 0x4
#define RAW_MODEL_MASK   0xf
#define RAW_MODEL( val ) GET_VAL(val, RAW_MODEL_OFFSET, RAW_MODEL_MASK)

#define RAW_FAMILY_OFFSET 0x8
#define RAW_FAMILY_MASK   0xf
#define RAW_FAMILY( val ) GET_VAL(val, RAW_FAMILY_OFFSET, RAW_FAMILY_MASK)

#define PROCESSOR_TYPE_OFFSET 0xc
#define PROCESSOR_TYPE_MASK   0x3
#define PROCESSOR_TYPE( val ) GET_VAL(val, PROCESSOR_TYPE_OFFSET, PROCESSOR_TYPE_MASK)

#define EXTENDED_MODEL_OFFSET 0x10
#define EXTENDED_MODEL_MASK   0xf
#define EXTENDED_MODEL( val ) GET_VAL(val, EXTENDED_MODEL_OFFSET, EXTENDED_MODEL_MASK)

#define EXTENDED_FAMILY_OFFSET 0x14
#define EXTENDED_FAMILY_MASK   0xff
#define EXTENDED_FAMILY( val ) GET_VAL(val, EXTENDED_FAMILY_OFFSET, EXTENDED_FAMILY_MASK)

/* Special value extractions */
#define FAMILY( val ) (RAW_FAMILY(val) + EXTENDED_FAMILY(val))
#define MODEL( val ) ((EXTENDED_MODEL(val) << 4) + RAW_MODEL(val))

enum CPUIDVal {
	STEPPING = 0,
	RAW_MODEL,
	MODEL, // (extended model << 4) + raw model
	RAW_FAMILY,
	FAMILY, // raw family + extended family
	PROCESSOR_TYPE,
	EXTENDED_MODEL,
	EXTENDED_FAMILY,
	RAW // Entire eax register
};

/*
 * CPU-ID querying functionality.
 */
namespace CPUID
{

#ifdef __GNUC__

/*
 * This API uses GCC's __get_cpuid feature to read CPU-ID information.  This
 * *most likely* returns CPU-ID information for the CPU on which the calling
 * thread is running.
 *
 * This does *not* allow us to query an arbitrary CPU's ID information.  This
 * shouldn't be a problem in current systems, where all CPUs are homogeneous.
 * However, we should switch to the device file interface exposed by Linux...
 */

/* General API (defined by GCC) */
unsigned getCPUIDVal(enum CPUIDVal field);

/* Low-level API */
retval_t readRawCPUID(unsigned cpuidLevel, unsigned* eax, unsigned* ebx,
					  unsigned* ecx, unsigned* edx);

#else

# error "CPU-ID via Linux device files is not implemented"

/* CPU-ID device file API (using Linux CPU-ID device file API) */
unsigned getCPUIDVal(int cpuidFD, enum CPUIDVal field);

#endif

}

#endif /* SRC_UTILITY_CPUID_H_ */
