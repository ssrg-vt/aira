/*
 * AMDManager.cpp
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#include <fstream>
#include <sstream>

/* Linux system information */
#include <sys/utsname.h>

#include "AMDManager.h"

#define toAMD( DevPtr ) ((AMDDevice*)DevPtr)

/*
 * Default constructor - initialize devices
 */
AMDManager::AMDManager()
{
	// TODO check for AMD GPU + APU management
	DEBUG("  AMDManager: available");
	_initializeDevices();
}

/*
 * Default destructor
 */
AMDManager::~AMDManager()
{
	// nothing for now...
}

void AMDManager::_initializeDevices()
{
	std::ifstream cpuinfo;
	std::string line;
	AMDDevice* pkg = NULL;
	int curCPU = -1, curPackage = -1;
	bool skip = false;

	// Find CPUs from /proc/cpuinfo
	cpuinfo.open("/proc/cpuinfo"); if(!cpuinfo.is_open()) return;
	while(std::getline(cpuinfo, line))
	{
		if(line.find("processor") != std::string::npos)
			curCPU++; // Add CPU after ensuring that it's AMD
		else if(line.find("vendor_id") != std::string::npos)
		{
			// Ensure AMD CPU
			if(line.substr(line.find(':') + 2) == "AuthenticAMD")
				skip = false;
			else
			{
				DEBUG("AMDManager: skipping non-AMD CPU " << curCPU);
				skip = true;
			}
		}
		else if(!skip && line.find("model name") != std::string::npos)
		{
			// Get CPU package info
			std::stringstream ss;
			std::ifstream packageFile;
			int package;

			ss << "/sys/devices/system/cpu/cpu" << curCPU << "/topology/physical_package_id";
			packageFile.open(ss.str().c_str());
			packageFile >> package;
			packageFile.close();

			if(package > curPackage)
			{
				if(pkg) _devices.push_back(pkg);
				pkg = new AMDDevice(AMD_CPU);
				curPackage = package;
			}

			// Set name & add CPU to device
			std::string name = line.substr(line.find(':') + 2);
			if(pkg->name() == "N/A") pkg->setName(name);
			pkg->addCPU(curCPU);
		}
		else if(!skip && line.find("cpu MHz") != std::string::npos)
		{
			if(pkg->clockSpeed() == 0)
			{
				std::stringstream ss;
				unsigned mhz;
				ss << line.substr(line.find(':') + 2);
				ss >> mhz;
				pkg->setClockSpeed(mhz);
			}
		}
	}

		// Add last device
	if(pkg) _devices.push_back(pkg);

	// TODO Populate GPU device(s)
	// TODO Populate APU device(s)

	_numDevices = _devices.size();

	// Check for power control
	for(unsigned i = 0; i < _numDevices; i++)
	{
		retval_t retval;
		switch(toAMD(_devices[i])->amdDevType())
		{
		case AMD_CPU:
			retval = toAMD(_devices[i])->initializeAMDPower();
			if(retval)
			{
				DEBUG(retvalStr[retval] << " (can't monitor power using AMD CPU power reporting)");
				continue;
			}
			break;
		case AMD_GPU:
			//TODO
			break;
		case AMD_APU:
			//TODO
			break;
		}
	}
}

/*
 * Return the Linux kernel version.
 *
 * @return the Linux kernel version
 */
std::string AMDManager::version() const
{
	struct utsname info;
	assert(!uname(&info));
	std::string version(info.release);
	return version;
}

/*
 * Return human-readable information about the specified device.
 *
 * @param dev the device number
 */
std::string AMDManager::getDeviceInfo(unsigned dev) const
{
	std::stringstream ss;

	if(dev >= _numDevices)
	{
		ss << "Invalid device number " << dev << ", must be 0";
		if(_numDevices > 1)
			ss << " - " << (_numDevices - 1);
	}
	else
	{
		ss << _devices[dev]->name() << " (" << _devices[dev]->clockSpeed() << "MHz)";
		if(toAMD(_devices[dev])->amdDevType() == AMD_CPU)
			ss << " - " << toAMD(_devices[dev])->numCPUs() << " core(s)";
	}

	return ss.str();
}

/*
 * Start power monitoring service for all AMD devices associated with the
 * manager.
 */
void AMDManager::startPowerMonitoring()
{
	for(unsigned i = 0; i < _devices.size(); i++)
	{
		if(_devices[i]->measurePower())
		{
			switch(toAMD(_devices[i])->amdDevType())
			{
			case AMD_CPU:
				assert(_devices[i]->startEnergyAccounting() == SUCCESS);
				break;
			case AMD_GPU:
				assert("AMD GPUs not yet supported!" && false);
				break;
			case AMD_APU:
				assert("AMD APUs not yet supported!" && false);
				break;
			}
		}
	}
}

/*
 * Measure power for each AMD device & update energy accounting.
 *
 * @param elapsedTime the time since last reading
 */
void AMDManager::measurePower(struct timespec& elapsedTime)
{
	for(unsigned i = 0; i < _devices.size(); i++)
	{
		if(_devices[i]->measurePower())
		{
			switch(toAMD(_devices[i])->amdDevType())
			{
			case AMD_CPU:
				measureAMDCPUPower(toAMD(_devices[i]), elapsedTime);
				break;
			case AMD_GPU:
				measureAMDGPUPower(toAMD(_devices[i]), elapsedTime);
				break;
			case AMD_APU:
				//TODO
				break;
			}
		}
	}
}

/*
 * Read the "Processor Power in TDP" & "Power Averaging Period" MSRs for the
 * AMD CPU to calculate instantaneous power, then accumulate reading for the
 * device.
 *
 * @param dev a pointer to the AMD CPU for which to read power
 * @param elapsedTime the elapsed time for the power reading
 */
void AMDManager::measureAMDCPUPower(AMDDevice* dev, struct timespec& elapsedTime)
{
	unsigned long long ppit, pap;
	retval_t ret = MSR::readMSR(dev->msrFD(), MSR_PROCESSOR_POWER_IN_TDP, &ppit);
	if(ret) ppit = 0;

	ret = MSR::readMSR(dev->msrFD(), MSR_POWER_AVERAGING_PERIOD, &pap);
	if(ret) pap = 0;

	dev->addEnergyConsumption(MSR_CURR_POWER_WATTS(ppit, pap), elapsedTime);
}

void AMDManager::measureAMDGPUPower(AMDDevice* dev, struct timespec& elapsedTime)
{
	// TODO
	ERR("Power measurement for AMD GPUs not yet implemented");
}
