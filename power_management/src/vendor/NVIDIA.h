/*
 * NVIDIA.h - definitions for NVIDIA devices & APIs (may be used in place of nvml.h)
 *
 *  Created on: May 1, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_VENDOR_NVIDIA_H_
#define SRC_VENDOR_NVIDIA_H_

/*
 * NVML return types (taken from NVIDIA GPU management & deployment docs)
 */
enum nvmlReturn_t {
	NVML_SUCCESS = 0, //The operation was successful.
	NVML_ERROR_UNINITIALIZED = 1, //NVML was not first initialized with nvmlInit().
	NVML_ERROR_INVALID_ARGUMENT = 2, //A supplied argument is invalid.
	NVML_ERROR_NOT_SUPPORTED = 3, //The requested operation is not available on target device.
	NVML_ERROR_NO_PERMISSION = 4, //The current user does not have permission for operation.
	NVML_ERROR_ALREADY_INITIALIZED = 5, // Deprecated: Multiple initializations are now allowed through ref counting.
	NVML_ERROR_NOT_FOUND = 6, //A query to find an object was unsuccessful.
	NVML_ERROR_INSUFFICIENT_SIZE = 7, //An input argument is not large enough.
	NVML_ERROR_INSUFFICIENT_POWER = 8, //A device's external power cables are not properly attached.
	NVML_ERROR_DRIVER_NOT_LOADED = 9, //NVIDIA driver is not loaded.
	NVML_ERROR_TIMEOUT = 10, //User provided timeout passed.
	NVML_ERROR_IRQ_ISSUE = 11, //NVIDIA Kernel detected an interrupt issue with a GPU.
	NVML_ERROR_LIBRARY_NOT_FOUND = 12, //NVML Shared Library couldn't be found or loaded.
	NVML_ERROR_FUNCTION_NOT_FOUND = 13, //Local version of NVML doesn't implement this function.
	NVML_ERROR_CORRUPTED_INFOROM = 14, //infoROM is corrupted
	NVML_ERROR_GPU_IS_LOST = 15, //The GPU has fallen off the bus or has otherwise become inaccessible.
	NVML_ERROR_UNKNOWN = 999
};

/* NVML clock types */
enum nvmlClockType_t {
	NVML_CLOCK_GRAPHICS = 0, // Graphics clock domain.
	NVML_CLOCK_SM = 1, // SM clock domain.
	NVML_CLOCK_MEM = 2, // Memory clock domain.
	NVML_CLOCK_COUNT
};

/* NVML P-state types */
enum nvmlPstates_t {
	NVML_PSTATE_0 = 0, // Performance state 0 -- Maximum Performance.
	NVML_PSTATE_1 = 1, // Performance state 1.
	NVML_PSTATE_2 = 2, // Performance state 2.
	NVML_PSTATE_3 = 3, // Performance state 3.
	NVML_PSTATE_4 = 4, // Performance state 4.
	NVML_PSTATE_5 = 5, // Performance state 5.
	NVML_PSTATE_6 = 6, // Performance state 6.
	NVML_PSTATE_7 = 7, // Performance state 7.
	NVML_PSTATE_8 = 8, // Performance state 8.
	NVML_PSTATE_9 = 9, // Performance state 9.
	NVML_PSTATE_10 = 10, // Performance state 10.
	NVML_PSTATE_11 = 11, // Performance state 11.
	NVML_PSTATE_12 = 12, // Performance state 12.
	NVML_PSTATE_13 = 13, // Performance state 13.
	NVML_PSTATE_14 = 14, // Performance state 14.
	NVML_PSTATE_15 = 15, // Performance state 15 -- Minimum Performance.
	NVML_PSTATE_UNKNOWN = 32 // Unknown performance state.
};

/* NVML temperature types */
enum nvmlTemperatureSensors_t {
	NVML_TEMPERATURE_GPU = 0, // Temperature sensor for the GPU die.
	NVML_TEMPERATURE_COUNT
};

/* NVML feature enabled types */
enum nvmlEnableState_t {
	NVML_FEATURE_DISABLED = 0, // Feature disabled.
	NVML_FEATURE_ENABLED // Feature enabled.
};

/* NVML device handle (opaque, don't need definition) */
typedef struct nvmlDevice_st* nvmlDevice_t;

/* NVML API - typedef the prototype for casting void* returned by dlsym */
// Initialize/shutdown
typedef nvmlReturn_t (*nvmlInit) (void);
typedef nvmlReturn_t (*nvmlShutdown) (void);

// System queries
typedef nvmlReturn_t (*nvmlSystemGetDriverVersion) (char* version, unsigned int length);

// Device queries
typedef nvmlReturn_t (*nvmlDeviceGetCount) (unsigned int* deviceCount);
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex) (unsigned int index, nvmlDevice_t* device);
typedef nvmlReturn_t (*nvmlDeviceGetName) (nvmlDevice_t device, char* name, unsigned int length);
typedef nvmlReturn_t (*nvmlDeviceGetClockInfo) (nvmlDevice_t device, nvmlClockType_t type, int* clock);
typedef nvmlReturn_t (*nvmlDeviceGetPowerManagementMode) (nvmlDevice_t device, nvmlEnableState_t* mode);
typedef nvmlReturn_t (*nvmlDeviceGetPowerState) (nvmlDevice_t device, nvmlPstates_t* pState);
typedef nvmlReturn_t (*nvmlDeviceGetPowerUsage) (nvmlDevice_t device, unsigned int* power);
typedef nvmlReturn_t (*nvmlDeviceGetTemperature) (nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int* temp);

#endif /* SRC_VENDOR_NVIDIA_H_ */
