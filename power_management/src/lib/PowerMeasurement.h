/*
 * PowerMeasurement.h - C interface to the power measurement library.
 *
 * Utilizes much of the infrastructure behind PowerManager to allow
 * applications to programmatically select & monitor power for devices for
 * periodic intervals.
 *
 * Implementation detail:  the library uses alarms (via POSIX timers) to
 * periodically measure power.  The library reserves SIGALRM, meaning that the
 * calling application must NOT use the signal for its own purposes!
 *
 * Additionally, the library installs a constructor which blocks the signal
 * from the main thread (and all child threads) automatically.  The calling
 * application must NOT unblock the signal!
 *
 *  Created on: May 13, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_LIB_POWERMEASUREMENT_H_
#define SRC_LIB_POWERMEASUREMENT_H_

#include <time.h>

#include "devices.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// General platform information
///////////////////////////////////////////////////////////////////////////////

/*
 * Returns the number of available managers.
 * @return the number of available managers
 */
size_t powerlib_num_managers();

/*
 * Returns the number of devices supported by the specified manager.
 * @param manager which manager to query
 * @return the number of devices supported by the manager
 */
size_t powerlib_num_devices(size_t manager);

/*
 * Return a string with information about the specified manager.
 * @param manager the manager for which to return information
 * @return a const char* with the manager's information
 */
const char* powerlib_manager_info(size_t manager);

/*
 * Return a string with information about the specified device.
 * @param manager the manager who supports the specified device
 * @param device the device for which to return information
 * @return a const char* with the device's information
 */
const char* powerlib_device_info(size_t manager, size_t device);

/*
 * Return the device type of the specified device.
 * @param manager the manager who supports the specified device
 * @param device the device for which to return the device type
 * @return the device type:
 * 	0 - CPU
 * 	1 - GPU
 * 	2 - APU
 *  3 - ACCELERATOR (e.g. Xeon Phi)
 *  999 - unknown
 */
enum devtype_t powerlib_device_type(size_t manager, size_t device);

/*
 * Return whether or not the specified device supports power measurement.
 * @param manager the manager who supports the specified device
 * @param device the device for which to return power measurement capability
 * @return true (1) if power measurement is supported, false (0) otherwise
 */
int powerlib_device_supported(size_t manager, size_t device);

///////////////////////////////////////////////////////////////////////////////
// Monitoring API
///////////////////////////////////////////////////////////////////////////////

/*
 * To reiterate - the library uses POSIX timers & SIGALRM to periodically
 * measure power.  Calling applications must NOT unblock the signal & must NOT
 * use it for their own needs!
 */

/* Opaque handles for monitoring power consumption */
typedef struct _powerlib_t _powerlib_t;
typedef _powerlib_t* powerlib_t;

/*
 * Initialize the library and return a handle used for managing library
 * power monitoring.  By default, all devices are disabled from power
 * monitoring.
 * @return a handle used for device power measurement, or NULL if there was a
 *         problem initializing
 */
powerlib_t powerlib_initialize();

/*
 * Shut down the library & clean up the handle (handle becomes invalid).
 * @param handle powerlib handle
 * @return false (0) if the library was successfully shut down, true (1)
 *         otherwise
 */
int powerlib_shutdown(powerlib_t handle);

/*
 * Start power monitoring for all of the previously added devices.  Register a
 * timer which sends a signal for power measurement.  The timer fires according
 * to the specified period.
 *
 * NOTE: previous measurement information is lost when calling
 * powerlib_start_monitoring!
 * @param handle powerlib handle
 * @param period the period with which to measure power
 * @return false (0) if monitoring was successfully started or true (1)
 *         otherwise
 */
int powerlib_start_monitoring(powerlib_t handle, struct timespec* period);

/*
 * Stop power monitoring, including the timer.
 * @param handle powerlib handle
 * @return false (0) if monitoring was successfully stopped or true (1)
 *         otherwise
 */
int powerlib_stop_monitoring(powerlib_t handle);

/*
 * Add a device for power measurement.
 * @param handle powerlib handle
 * @param manager the manager who supports the specified device
 * @param device the device for which to monitor power
 * @return 1 if the device was added, 0 if it has already been added (or if it
 *         cannot measure power) or -1 if there was a problem adding the device
 */
int powerlib_add_device(powerlib_t handle, size_t manager, size_t device);

/*
 * Add all devices supported by the specified manager for power measurement.
 * @param handle powerlib handle
 * @param manager the manager whose devices will monitor power
 * @return the number of devices added or -1 if there was a problem adding the
 *         manager's devices
 */
int powerlib_add_manager(powerlib_t handle, size_t manager);

/*
 * Add all devices which identify as the specified type for power measurement.
 * @param handle powerlib handle
 * @param type the type of devices who will monitor power
 * @return the number of devices added or -1 if there was a problem adding the
 *         devices
 */
int powerlib_add_devtype(powerlib_t handle, devtype_t type);

/*
 * Add all devices supported by the library for power measurement.
 * @param handle powerlib handle
 * @return the number of devices added or -1 if there was a problem adding all
 *         devices
 */
int powerlib_add_all_devices(powerlib_t handle);

/*
 * Remove a device from power measurement.
 * @param handle powerlib handle
 * @param manager the manager who supports the specified device
 * @param device the device for which to disable power monitoring
 * @return 1 if the device was removed, 0 if it was not previously added or -1
 *         if there was a problem removing the device
 */
int powerlib_remove_device(powerlib_t handle, size_t manager, size_t device);

/*
 * Remove all devices which identify as the specified type from power
 * measurement.
 * @param handle powerlib handle
 * @param type the type of devices who will no longer monitor power
 * @return the number of devices removed or -1 if there was a problem removing
 *         the devices
 */
int powerlib_remove_devtype(powerlib_t handle, devtype_t type);

/*
 * Remove all devices supported by the specified manager from power measurement.
 * @param handle powerlib handle
 * @param manager the manager whose devices will no longer monitor power
 * @return the number of devices removed or -1 if there was a problem removing
 *         the manager's devices
 */
int powerlib_remove_manager(powerlib_t handle, size_t manager);

/*
 * Remove all devices from power measurement.
 * @param handle powerlib handle
 * @return the number of devices removed or -1 if there was a problem removing
 *         all devices
 */
int powerlib_remove_all_devices(powerlib_t handle);

/*
 * Return whether or not a device is enabled for power measurement.
 * @param handle powerlib handle
 * @param manager the manager who supports the specified device
 * @param device the device for which to check if power monitoring is enabled
 * @return true (1) if power monitoring is enabled, false (0) if not or -1 if
 *         there was a problem
 */
int powerlib_device_added(powerlib_t handle, size_t manager, size_t device);

/*
 * Return a string containing a power monitoring summary for devices whose
 * power was measured.
 * Note: Application is responsible for freeing string!
 * @param handle powerlib handle
 * @return a const char* containing a summary of power measurements, or NULL if
 *         there was a problem.  Application is responsible for freeing
 *         returned string!
 */
const char* powerlib_summarize_measurements(powerlib_t handle);

/*
 * Return the number of periods for which power was monitored.  Equivalent to
 * the number of times the timer fired.
 * @param handle powerlib handle
 * @return the number of times the timer fired, or -1 if there was a problem
 */
int powerlib_num_periods_measured(powerlib_t handle);

/*
 * Return the energy consumed by the specified device during monitoring.
 * @param handle powerlib handle
 * @param manager the manager who supports the specified device
 * @param device the device for which to return energy consumption
 * @return the amount of energy consumed
 */
double powerlib_energy(powerlib_t handle, size_t manager, size_t device);

/*
 * Return the average power consumption of the specified device during
 * monitoring.
 * @param handle powerlib handle
 * @param manager the manager who supports the specified device
 * @param device the device for which to return average power consumption
 * @return the average power consumption
 */
double powerlib_avg_power(powerlib_t handle, size_t manager, size_t device);

/*
 * Structures used to enumerate per-component power/energy information.
 */
typedef struct component_info_t {
	const char* name;
	double energy;
	double power;
} component_info_t;

typedef struct detailed_info_t {
	size_t num_fields;
	component_info_t* fields;
} detailed_info_t;

/*
 * Return whether or not the specified device has per-component power & energy
 * breakdowns.
 */
int powerlib_has_per_component(powerlib_t handle, size_t manager, size_t device);

detailed_info_t* powerlib_get_detailed_info(powerlib_t handle, size_t manager, size_t device);

#ifdef __cplusplus
}
#endif

#endif /* SRC_LIB_POWERMEASUREMENT_H_ */
