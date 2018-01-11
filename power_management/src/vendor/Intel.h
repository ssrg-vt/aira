/*
 * Intel.h
 *
 *  Created on: May 6, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_VENDOR_INTEL_H_
#define SRC_VENDOR_INTEL_H_

/*
 * Intel Running-Average Power Limit (RAPL) defines
 *
 * Taken from Intel 64 and IA-32 Architectures Software Developer's Manual
 * http://www.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html
 *
 * RAPL interface is described in the System Programming Guide, Section 14.9.1
 * MSRs interface is described in the System Programming Guide, Chapter 35
 *
 * Taken from PAPI 5.4.1:
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */

/* RAPL defines */
#define MSR_RAPL_POWER_UNIT 0x606

/* Package */
#define MSR_PKG_RAPL_POWER_LIMIT 0x610
#define MSR_PKG_ENERGY_STATUS    0x611
#define MSR_PKG_PERF_STATUS      0x613
#define MSR_PKG_POWER_INFO       0x614

/* PP0 */
#define MSR_PP0_POWER_LIMIT   0x638
#define MSR_PP0_ENERGY_STATUS 0x639
#define MSR_PP0_POLICY        0x63A
#define MSR_PP0_PERF_STATUS   0x63B

/* PP1 */
#define MSR_PP1_POWER_LIMIT   0x640
#define MSR_PP1_ENERGY_STATUS 0x641
#define MSR_PP1_POLICY        0x642

/* DRAM */
#define MSR_DRAM_POWER_LIMIT   0x618
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_DRAM_PERF_STATUS   0x61B
#define MSR_DRAM_POWER_INFO    0x61C

/* RAPL bitmasks */
#define POWER_UNIT_OFFSET 0
#define POWER_UNIT_MASK   0x0f
#define POWER_UNIT( val ) GET_VAL(val, POWER_UNIT_OFFSET, POWER_UNIT_MASK)

#define ENERGY_UNIT_OFFSET 0x08
#define ENERGY_UNIT_MASK   0x1f
#define ENERGY_UNIT( val ) GET_VAL(val, ENERGY_UNIT_OFFSET, ENERGY_UNIT_MASK)

#define ENERGY_VAL_OFFSET 0x0
#define ENERGY_VAL_MASK   0xffffffff
#define ENERGY_VAL( val ) GET_VAL(val, ENERGY_VAL_OFFSET, ENERGY_VAL_MASK)

#define TIME_UNIT_OFFSET 0x10
#define TIME_UNIT_MASK   0x0f
#define TIME_UNIT( val ) GET_VAL(val, TIME_UNIT_OFFSET, TIME_UNIT_MASK)

/* RAPL POWER UNIT MASKS */
#define POWER_INFO_UNIT_MASK      0x7fff
#define THERMAL_SHIFT             0
#define MINIMUM_POWER_SHIFT       16
#define MAXIMUM_POWER_SHIFT       32
#define MAXIMUM_TIME_WINDOW_SHIFT 48

#endif /* SRC_VENDOR_INTEL_H_ */
