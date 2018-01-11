/*
 * CPUID.cpp
 *
 *  Created on: May 13, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <stdint.h>

#include "CPUID.h"
#include "cpuid.h"

#ifdef __GNUC__

/*
 * Query CPU-ID information & return the specified field.
 *
 * @param field the CPU-ID field
 * @return SUCCESS if the value was successfully read, UINT32_MAX otherwise
 */
unsigned CPUID::getCPUIDVal(enum CPUIDVal field)
{
	unsigned eax, ebx, ecx, edx;
	if(readRawCPUID(1, &eax, &ebx, &ecx, &edx))
		return UINT32_MAX;

	switch(field)
	{
	case STEPPING:
		return STEPPING(eax);
	case RAW_MODEL:
		return RAW_MODEL(eax);
	case MODEL:
		return MODEL(eax);
	case RAW_FAMILY:
		return RAW_FAMILY(eax);
	case FAMILY:
		return FAMILY(eax);
	case PROCESSOR_TYPE:
		return PROCESSOR_TYPE(eax);
	case EXTENDED_MODEL:
		return EXTENDED_MODEL(eax);
	case EXTENDED_FAMILY:
		return EXTENDED_FAMILY(eax);
	case RAW:
		return eax;
	default:
		ERR("Requested unknown CPU-ID field");
	}
}

/*
 * Read the specified CPUID file & return the raw data.
 *
 * @param level the CPU-ID level to query
 * @param eax a pointer to where CPU-ID information for register eax is stored
 * @param ebx a pointer to where CPU-ID information for register ebx is stored
 * @param ecx a pointer to where CPU-ID information for register ecx is stored
 * @param edx a pointer to where CPU-ID information for register edx is stored
 * @return SUCCESS if CPU-ID was successfully read, an error code otherwise
 */
retval_t CPUID::readRawCPUID(unsigned level, unsigned* eax, unsigned* ebx,
							 unsigned* ecx, unsigned* edx)
{
	if(!__get_cpuid(level, eax, ebx, ecx, edx))
		return COULD_NOT_READ_CPUID;
	return SUCCESS;
}

#endif
