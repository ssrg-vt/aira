#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <CL/cl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file includes all data that must be defined per-system in order to
 * support various predictor & load-balancer actions.  Systems should be
 * defined in their own file in the 'systems' folder in the root, and selected
 * at compile time using:
 *
 *   make system=<filename>
 */

///////////////////////////////////////////////////////////////////////////////
// Predictor output slot information
///////////////////////////////////////////////////////////////////////////////

/* Number of predictor output slots */
extern const size_t prediction_slots;

/* Default CPU device in the system (used to convert statistics into ratios) */
extern const size_t default_cpu;

/* Device types of predictor output slots */
extern const cl_device_type device_types[];

/* Map prediction output slots to struct resource_allocation */
extern const struct resource_alloc system_devices[];

///////////////////////////////////////////////////////////////////////////////
// Benchmark statistics
///////////////////////////////////////////////////////////////////////////////

/* NPB runtime statistics */
extern const float rt_mean;
extern const float rt_max;
extern const float rt_min;

/* NPB benchmark runtimes on each architecture -- values are in seconds. */
extern const float runtime [][40];

/* NPB energy consumption statistics */
extern const float energy_mean;
extern const float energy_max;
extern const float energy_min;

/* NPB benchmark energy consumption on each architecture -- values are in
 * joules. */
extern const float energy [][40];

#ifdef __cplusplus
}
#endif

#endif /* _SYSTEM_H */

