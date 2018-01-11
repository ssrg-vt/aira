/*
 * common.h - common & useful definitions
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "config.h"
#include "devices.h"

#ifdef __cplusplus

#include <iostream>
#include <cassert>

#ifdef _VERBOSE
/* Standard debugging */
# define DEBUG( msg ) std::cerr << "[PM-debug] " << msg << std::endl

/* Useful for multi-part debugging information */
# define DEBUG_NOEOL( msg ) std::cerr << "[PM-debug] " << msg
# define DEBUG_CONT( msg ) std::cerr << msg
#else
# define DEBUG( msg )
# define DEBUG_NOEOL( msg )
# define DEBUG_CONT( msg )
#endif

/* Standard printing */
#define INFO( msg ) std::cout << "[PM-info] " << msg << std::endl
#define WARN( msg ) std::cerr << "[PM-WARN] WARNING: " << msg << std::endl
#define ERR( msg ) do { \
	std::cerr << "[PM-ERR] ERROR: " << msg << std::endl; \
	assert(false); \
} while(1);

#endif

/* Return codes used by PowerManager */
typedef enum retval_t {
	SUCCESS = 0,

	// CPUID device file return codes
	COULD_NOT_READ_CPUID,

	// Device return codes
	INVALID_DEV_NUM,
	ACCOUNTING_ALREADY_STARTED,
	ACCOUNTING_NOT_STARTED,
	DEV_CANNOT_MEASURE_POWER,
	ALREADY_ENABLED,
	ALREADY_DISABLED,

	// Model-specific registers (MSR) device file return codes
	MSR_FILE_DOES_NOT_EXIST,
	COULD_NOT_OPEN_MSR,
	COULD_NOT_READ_MSR,
	COULD_NOT_CLOSE_MSR,

	// Intel RAPL device return codes
	UNKNOWN_INTEL_CPU,
	CPU_NO_RAPL_SUPPORT,
	COULD_NOT_GET_ENERGY_DIVISOR,
	COULD_NOT_READ_PKG_STATUS,
	COULD_NOT_READ_PP0_STATUS,
	COULD_NOT_READ_PP1_STATUS,
	COULD_NOT_READ_DRAM_STATUS,

	// AMD device return codes
	POWER_MSRS_NOT_AVAILABLE,

	// Daemon return codes
	NOT_ROOT,
	COULD_NOT_FORK,
	COULD_NOT_CREATE_PGROUP,
	COULD_NOT_CHANGE_WD,
	COULD_NOT_REGISTER_SIGNALS,
	COULD_NOT_WRITE_PID,
	COULD_NOT_OPEN_LOGFILE,
	COULD_NOT_REDIRECT_STD_STREAMS,
	COULD_NOT_DELETE_PID,
	COULD_NOT_START,

	UNKNOWN
} retval_t;

/* Human-readable return code strings */
extern const char* retvalStr[];

/* Value extraction */
#define GET_VAL(val, offset, mask) ((val >> offset) & mask)

#endif /* COMMON_H_ */
