/*
 * arch_selector.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "partition_kernels_common.h"
#include "pragma_handling.h"
#include "arch_selector.h"

/*
 * Default constructor.  Build a list of of requested architectures for a kernel
 * and check to make sure that it is partitionable onto the requested
 * architectures.
 */
ArchSelector::ArchSelector(PragmaParser& requested, PragmaParser& allowed) :
	useGpu(false),
	useMpi(false)
{
	set<string> archs = requested.getNames();
	set<string>::const_iterator archIt;
	string msg;
	for(archIt = archs.begin(); archIt != archs.end(); archIt++)
	{
		if(*archIt == GPU_STR)
			useGpu = true;
		else if(*archIt == MPI_STR)
			useMpi = true;
		else if(*archIt == X86_STR)
			continue;
		else
		{
			msg = "Unknown architecture: " + *archIt;
			WARNING(TOOL, msg);
		}
	}

	//Check to make sure requested arches are partitionable
	if(useGpu && allowed.getNames().find("gpu") == allowed.getNames().end())
	{
		ERROR(TOOL, "\tRequested GPU partition for incompatible kernel!");
		useGpu = false;
	}
	if(useMpi && allowed.getNames().find("mpi") == allowed.getNames().end())
	{
		ERROR(TOOL, "\tRequested MPI partition for incompatible kernel!");
		useMpi = false;
	}
}

/*
 * Return whether or not this kernel is to be partitioned onto the GPU.
 */
bool ArchSelector::partitionOntoGpu()
{
	return useGpu;
}

/*
 * Return whether or not this kernel is to be partitioned onto an MPI partition.
 */
bool ArchSelector::partitionOntoMpi()
{
	return useMpi;
}
