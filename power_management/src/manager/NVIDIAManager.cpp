/*
 * NVIDIAManager.cpp
 *
 *  Created on: Apr 29, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <sstream>
#include <dlfcn.h>

#include "NVIDIAManager.h"
#include "NVIDIADevice.h"

#define nvmlDev( devPtr ) (((NVIDIADevice*)devPtr)->getDev())

/*
 * Default constructor - load management library & initialize for NVIDIA
 * devices.
 */
NVIDIAManager::NVIDIAManager()
{
	_mlHandle = dlopen(vendorLibraries[NVIDIA], RTLD_LAZY); assert(_mlHandle);

	DEBUG("  NVIDIAManager: available (loading " << vendorLibraries[NVIDIA] << ")");
	_loadFunctions();
	assert(_init() == NVML_SUCCESS); // NVML init
	_initializeDevices();
}

/*
 * Default destructor - shut down NVML & close management library.
 */
NVIDIAManager::~NVIDIAManager()
{
	assert(_shutdown() == NVML_SUCCESS); // NVML shutdown
	dlclose(_mlHandle);
}

/*
 * Load management functions using dynamic linker.
 */
void NVIDIAManager::_loadFunctions()
{
	assert(_mlHandle);

	// Initialization & cleanup
	_init = (nvmlInit)dlsym(_mlHandle, "nvmlInit"); assert(_init);
	_shutdown = (nvmlShutdown)dlsym(_mlHandle, "nvmlShutdown"); assert(_shutdown);

	// Queries
	_version = (nvmlSystemGetDriverVersion)dlsym(_mlHandle, "nvmlSystemGetDriverVersion"); assert(_version);
	_getNumDevices = (nvmlDeviceGetCount)dlsym(_mlHandle, "nvmlDeviceGetCount"); assert(_getNumDevices);
	_getDevice = (nvmlDeviceGetHandleByIndex)dlsym(_mlHandle, "nvmlDeviceGetHandleByIndex"); assert(_getDevice);
	_getName = (nvmlDeviceGetName)dlsym(_mlHandle, "nvmlDeviceGetName"); assert(_getName);
	_getClock = (nvmlDeviceGetClockInfo)dlsym(_mlHandle, "nvmlDeviceGetClockInfo"); assert(_getClock);
	_powerEnabled = (nvmlDeviceGetPowerManagementMode)dlsym(_mlHandle, "nvmlDeviceGetPowerManagementMode"); assert(_powerEnabled);
	_getPstate = (nvmlDeviceGetPowerState)dlsym(_mlHandle, "nvmlDeviceGetPowerState"); assert(_getPstate);
	_getPower = (nvmlDeviceGetPowerUsage)dlsym(_mlHandle, "nvmlDeviceGetPowerUsage"); assert(_getPower);
	_getTemperature = (nvmlDeviceGetTemperature)dlsym(_mlHandle, "nvmlDeviceGetTemperature"); assert(_getTemperature);

	// Actions
}

/*
 * Initialize NVIDIA devices using NVML library.
 */
void NVIDIAManager::_initializeDevices()
{
	NVIDIADevice* dev;
	nvmlDevice_t handle;

	_getNumDevices(&_numDevices);
	for(unsigned i = 0; i < _numDevices; i++)
	{
		_getDevice(i, &handle); assert(handle);
		dev = new NVIDIADevice(handle);

		// Get name & clock speed
		char name[64];
		_getName(dev->getDev(), name, 64);
		std::string strName(name);
		dev->setName(strName);

		int clock;
		_getClock(dev->getDev(), NVML_CLOCK_SM, &clock);
		dev->setClockSpeed(clock);

		// Check if we can read/manage power for device
		nvmlEnableState_t power;
		_powerEnabled(dev->getDev(), &power);
		if(!power)
		{
			DEBUG("found unsupported device: " << strName << " (can't monitor/manage power using NVML)");
			dev->canMeasurePower(false);
		}
		_devices.push_back(dev);
	}

	_numDevices = _devices.size();
}

/*
 * Return a string with the installed driver's version.
 *
 * @return a string containing the driver version
 */
std::string NVIDIAManager::version() const
{
	const size_t size = 64;
	char version[size];
	_version(version, size);
	return std::string(version);
}

/*
 * Return human-readable information about the specified device.
 *
 * @param dev the device number
 */
std::string NVIDIAManager::getDeviceInfo(unsigned dev) const
{
	std::stringstream ss;

	if(dev >= _numDevices)
	{
		ss << "Invalid device number, must be 0";
		if(_numDevices > 1)
			ss << " - " << (_numDevices - 1);
	}
	else
	{
		ss << _devices[dev]->name() + " (";
		unsigned clock = _devices[dev]->clockSpeed();
		if(clock)
			ss << _devices[dev]->clockSpeed() << "MHz)";
		else
			ss << "N/A)";
	}

	return ss.str();
}

/*
 * Re-initialize NVML library (if needed) & start power monitoring service for
 * all devices associated with the manager.
 */
void NVIDIAManager::startPowerMonitoring()
{
	assert(_init() == NVML_SUCCESS);
	for(unsigned i = 0; i < _devices.size(); i++)
	{
		if(_devices[i]->measurePower())
			assert(_devices[i]->startEnergyAccounting() == SUCCESS);
	}
}

/*
 * Measure power for each NVIDIA device & update energy accounting.
 *
 * @param elapsedTime the time since last reading
 */
void NVIDIAManager::measurePower(struct timespec& elapsedTime)
{
	unsigned power;
	for(unsigned i = 0; i < _devices.size(); i++)
	{
		if(_devices[i]->measurePower())
		{
			if(_getPower(nvmlDev(_devices[i]), &power) != NVML_SUCCESS)
				continue;
			_devices[i]->addEnergyConsumption(power / 1000, elapsedTime);
		}
	}
}
