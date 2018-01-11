/*
 * IntelManager.cpp
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <fstream>
#include <sstream>
#include <stdint.h>

/* Linux system information */
#include <sys/utsname.h>

#include "IntelManager.h"
#include "Intel.h"
#include "MSR.h"

/* Object type conversions */
#define toIntel( devPtr ) ((IntelDevice*)devPtr)
#define toRAPL( devPtr ) ((IntelRAPLDevice*)devPtr)
#define toXeonPhi( devPtr ) () // TODO

/*
 * Default constructor - initialize devices.
 */
IntelManager::IntelManager()
{
	DEBUG("  IntelManager: available");
	_initializeDevices();
}

/*
 * Default destructor - close any RAPL file descriptors
 */
IntelManager::~IntelManager()
{
	// nothing for now...
}

/*
 * Create objects for all Intel devices in the system.
 *
 * Intel CPUs: Parse /proc/cpuinfo & /sys/devices/system/cpu to create Intel
 * CPU devices.  A device is created per-package, the measurement granularity
 * for RAPL.
 *
 * Intel Xeon Phi: TODO
 *
 * Note: IntelManager ONLY manages Intel devices (no ARM/AMD/etc)!
 */
void IntelManager::_initializeDevices()
{
	// Initialize Intel CPU/RAPL devices
	std::ifstream cpuinfo;
	std::string line;
	IntelRAPLDevice* pkg = NULL;
	int curCPU = -1, curPackage = -1;
	bool skip = false;

	cpuinfo.open("/proc/cpuinfo"); if(!cpuinfo.is_open()) return;
	while(std::getline(cpuinfo, line))
	{
		if(line.find("processor") != std::string::npos)
			curCPU++; // Add CPU after ensuring that it's Intel
		else if(line.find("vendor_id") != std::string::npos)
		{
			if(line.substr(line.find(':') + 2) == "GenuineIntel") // Ensure Intel CPU
				skip = false;
			else
			{
				DEBUG("IntelManager: skipping non-Intel CPU " << curCPU);
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
				pkg = new IntelRAPLDevice();
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
	if(pkg) _devices.push_back(pkg);
	cpuinfo.close();

	// TODO Populate Xeon Phi device(s)

	_numDevices = _devices.size();

	// Check for power control
	for(unsigned i = 0; i < _numDevices; i++)
	{
		retval_t retval;
		switch(toIntel(_devices[i])->intelDevType())
		{
		case INTEL_RAPL:
			retval = toRAPL(_devices[i])->initializeRAPL();
			if(retval)
			{
				DEBUG(retvalStr[retval] << " (can't monitor power using RAPL)");
				continue;
			}
			break;
		case XEON_PHI:
			// TODO Xeon Phi
			break;
		}
	}
}

/*
 * Return the Linux kernel version.
 *
 * @return the Linux kernel version
 */
std::string IntelManager::version() const
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
std::string IntelManager::getDeviceInfo(unsigned dev) const
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
		if(toIntel(_devices[dev])->intelDevType() == INTEL_RAPL)
			ss << " - " << toRAPL(_devices[dev])->numCPUs() << " core(s)";
	}

	return ss.str();
}

/*
 * Start power monitoring service for all Intel devices associated with the
 * manager.
 */
void IntelManager::startPowerMonitoring()
{
	for(unsigned i = 0; i < _devices.size(); i++)
	{
		if(_devices[i]->measurePower())
		{
			switch(toIntel(_devices[i])->intelDevType())
			{
			case INTEL_RAPL:
				assert(_devices[i]->startEnergyAccounting() == SUCCESS);
				break;
			case XEON_PHI:
				assert("Xeon Phi not yet implemented" && false);
				break;
			}
		}
	}
}

/*
 * Measure power for each Intel device & update energy accounting.
 *
 * @param elapsedTime the time since last reading
 */
void IntelManager::measurePower(struct timespec& elapsedTime)
{
	for(unsigned i = 0; i < _devices.size(); i++)
	{
		if(_devices[i]->measurePower())
		{
			switch(toIntel(_devices[i])->intelDevType())
			{
			case INTEL_RAPL:
				assert(toRAPL(_devices[i])->addEnergyConsumption(elapsedTime) == SUCCESS);
				break;
			case XEON_PHI:
				_devices[i]->addEnergyConsumption(_measureXeonPhiPower(_devices[i]), elapsedTime);
				break;
			}
		}
	}
}

unsigned IntelManager::_measureXeonPhiPower(Device* dev)
{
	//TODO
	ERR("measuring Xeon Phi power is unimplemented");
}
