/*
 * VendorManager.cpp
 *
 *  Created on: Apr 29, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <dlfcn.h>

#include "VendorManager.h"

/* Definition of vendor names */
const char* vendorNames[] = {
	"Intel (Linux)",
	"AMD (Linux)",
	"NVIDIA",
	"N/A"
};

/* Definition of vendor management libraries */
const char* vendorLibraries[] = {
	"N/A",
	"N/A",
	"libnvidia-ml.so",
	"N/A"
};

/*
 * Get a constant reference to a device for querying information about the
 * device.
 *
 * @param dev the device number
 * @return a const pointer to the specified device
 */
const Device* VendorManager::getDevice(unsigned dev) const
{
	return _devices[dev];
}

/*
 * Enable a device for power measurement.
 *
 * @param dev the device number for which to enable power measurement
 * @return SUCCESS if it was enabled, an error code otherwise
 */
retval_t VendorManager::enableDevice(unsigned dev)
{
	if(dev >= _devices.size())
		return INVALID_DEV_NUM;
	if(!_devices[dev]->canMeasurePower())
		return DEV_CANNOT_MEASURE_POWER;
	if(_devices[dev]->isMeasurementEnabled())
		return ALREADY_ENABLED;
	_devices[dev]->enablePowerMeasurement();
	return SUCCESS;
}

/*
 * Disable a device from power measurement.
 *
 * @param dev the device number for which to disable power measurement
 * @return SUCCESS if it was disabled, an error code otherwise
 */
retval_t VendorManager::disableDevice(unsigned dev)
{
	if(dev >= _devices.size())
		return INVALID_DEV_NUM;
	if(!_devices[dev]->isMeasurementEnabled())
		return ALREADY_DISABLED;
	_devices[dev]->disablePowerMeasurement();
	return SUCCESS;
}

/*
 * Reset any previously collected energy/power information.
 */
void VendorManager::resetEnergyMeasurements()
{
	for(unsigned i = 0; i < _devices.size(); i++)
		assert(_devices[i]->resetDeviceExpenditure() == SUCCESS);
}

/*
 * Stop power monitoring service for all devices associated with the manager.
 */
void VendorManager::stopPowerMonitoring()
{
	for(unsigned i = 0; i < _devices.size(); i++)
		if(_devices[i]->measurePower())
			assert(_devices[i]->stopEnergyAccounting() == SUCCESS);
}

/*
 * Probe the system for the specified vendor's management library.
 *
 * @param v the vendor for which to check
 * @return true if vendor management is available, false otherwise
 */
bool VendorManager::isAvailable(enum vendor v)
{
	switch(v) {
	case INTEL:  // Supported "out of the box" if running on Linux
#ifdef __linux__
		return true;
#else
		return false;
#endif
	case AMD:
#ifdef __linux__
		return true;
#else
		return false;
#endif
	case NVIDIA:
		return _probeLibrary(vendorLibraries[NVIDIA]);
	default:
		return false;
	}
}

/*
 * See if a vendor is supported by probing the specified library using the
 * dynamic linker.
 *
 * @param lib a string indicating the library to probe
 * @return true if the library is dynamically linkable, or false otherwise
 */
bool VendorManager::_probeLibrary(const char* lib)
{
	void* tmp = dlopen(lib, RTLD_LAZY);
	if(tmp)
	{
		dlclose(tmp);
		return true;
	}
	else
		return false;
}
