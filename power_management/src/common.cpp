/*
 * common.cpp
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include "common.h"

const char* retvalStr[] = {
	"success",

	// CPUID device file return codes
	"could not read CPUID device file",

	// Device return codes
	"specified an invalid device",
	"device accounting has already been started",
	"device accounting has not been started",
	"device cannot measure power",
	"power measurement is already enabled",
	"power measurement is already disabled",

	// Model-specific registers (MSR) device file return codes
	"model-specific registers device file does not exist - make sure \"msr\" module is inserted",
	"could not open model-specific registers device file",
	"could not read model-specific registers device file",
	"could not close model-specific registers device file",

	// Intel RAPL device return codes
	"unknown Intel CPU - may support RAPL but not yet implemented",
	"Intel CPU does not have RAPL support",
	"could not get RAPL energy units",
	"could not read package energy status",
	"could not read power plane 0 energy status",
	"could not read power plane 1 energy status",
	"could not read DRAM energy status",

	// AMD device return codes
	"model-specific registers for processor power cannot be read by host software",

	// Daemon return codes
	"manager must be run as root",
	"could not fork daemon process",
	"daemon - could not create new process group",
	"daemon - could not change working directory",
	"daemon - could not register signals",
	"daemon - could not write PID to file",
	"daemon - could not open log file",
	"daemon - could not redirect stdout/stderr to log file",
	"daemon - could not delete PID file",
	"could not start daemon",

	"unknown"
};
