/*
 * IntelRAPLDevice.cpp
 *
 *  Created on: May 13, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <sstream>
#include <stdint.h>

#include "IntelRAPLDevice.h"
#include "CPUID.h"
#include "MSR.h"

/*
 * Default constructor - detect CPU type (client/server) to enable supported
 * energy domains.
 */
IntelRAPLDevice::IntelRAPLDevice()
	: IntelDevice(INTEL_RAPL), _class(NOT_YET_DETECTED), _msrFD(-1), _esu(1.0),
	  _pp1Enabled(false), _dramEnabled(false), _pp0Power(0), _pp1Power(0),
	  _dramPower(0), _pp0Energy(0), _pp1Energy(0), _dramEnergy(0), _prevPkg(0),
	  _prevPP0(0), _prevPP1(0), _prevDRAM(0)
{
	// nothing for now...
}

/*
 * Default destructor - close MSR file descriptor.
 */
IntelRAPLDevice::~IntelRAPLDevice()
{
	MSR::closeMSR(_msrFD);
}

/*
 * Start energy accounting for the Intel device.  Read all available energy
 * statuses & timestamp the start.
 *
 * @return SUCCESS if accounting was started, an error code otherwise
 */
retval_t IntelRAPLDevice::startEnergyAccounting()
{
	if(_started) return ACCOUNTING_ALREADY_STARTED;

	retval_t ret = _measureRAPLEnergy(_prevPkg, _prevPP0, _prevPP1, _prevDRAM);
	if(ret) return ret;
	_started = true;
	clock_gettime(CLOCK_REALTIME, &_start);
	return SUCCESS;
}

/*
 * Account for power/energy consumption based on RAPL readings.  Converts the
 * reading into energy consumed and average power consumption for the elapsed
 * time interval.
 *
 * @param time elapsed time
 * @return SUCCESS if accounting has been started & statuses were read, an
 *         error code otherwise
 */
retval_t IntelRAPLDevice::addEnergyConsumption(struct timespec& time)
{
	if(!_started) return ACCOUNTING_NOT_STARTED;

	unsigned pkg = 0, pp0 = 0, pp1 = 0, dram = 0;
	retval_t ret = _measureRAPLEnergy(pkg, pp0, pp1, dram);
	if(ret) return ret;
	_updateRAPLAccounting(pkg, _prevPkg, _energy, _power, time);
	_updateRAPLAccounting(pp0, _prevPP0, _pp0Energy, _pp0Power, time);
	_updateRAPLAccounting(pp1, _prevPP1, _pp1Energy, _pp1Power, time);
	_updateRAPLAccounting(dram, _prevDRAM, _dramEnergy, _dramPower, time);
	return SUCCESS;
}


/*
 * Reset Intel RAPL device's energy accounting.
 *
 * @return SUCCESS, always
 */
retval_t IntelRAPLDevice::resetDeviceExpenditure()
{
	_power = 0;
	_pp0Power = 0;
	_pp1Power = 0;
	_dramPower = 0;
	_energy = 0;
	_pp0Energy = 0;
	_pp1Energy = 0;
	_dramEnergy = 0;
	_prevPkg = 0;
	_prevPP0 = 0;
	_prevPP1 = 0;
	_prevDRAM = 0;
	return SUCCESS;
}

/*
 * Report power readings for enabled domains.
 */
std::string IntelRAPLDevice::power() const
{
	std::stringstream power;

	power << _power << "W (package), " << _pp0Power << "W (PP0), ";

	if(_pp1Enabled && _dramEnabled)
		power << _pp1Power << "W (PP1), " << _dramPower << "W (DRAM)";
	else if(_pp1Enabled)
		 power << _pp1Power << "W (PP1)";
	else if(_dramEnabled)
		power << _dramPower << "W (DRAM)";

	return power.str();
}

/*
 * Initialize RAPL for the CPU package - check for RAPL availability & open the
 * appropriate model-specific registers (MSR) device file.
 *
 * @return SUCCESS if the MSR device file was opened successfully, an error
 *         code otherwise
 */
retval_t IntelRAPLDevice::initializeRAPL()
{
	retval_t ret;

	// Check CPU family for RAPL availability
	if(CPUID::getCPUIDVal(FAMILY) != 6)
	{
		_canMeasurePower = false;
		return CPU_NO_RAPL_SUPPORT;
	}

	// Check CPU model for RAPL availability.  Set class & enable appropriate
	// energy readings
	unsigned model = CPUID::getCPUIDVal(MODEL);
	if(model == 0x2A || // SandyBridge
	   model == 0x3a || // IvyBridge
	   model == 0x3c || model == 0x45 || model == 0x46) // Haswell
	{
		_class = CLIENT;
		_pp1Enabled = true;
		if(model == 0x3c || model == 0x45 || model == 0x46)
			_dramEnabled = true;
	}
	else if(model == 0x2d ||
			model == 0x3e ||
			model == 0x3f)
	{
		_class = SERVER;
		_dramEnabled = true;
	}
	else
	{
		_canMeasurePower = false;
		return UNKNOWN_INTEL_CPU;
	}

	// Open file descriptor
	ret = MSR::openMSR(_cpus[0], _msrFD);
	if(ret)
	{
		_canMeasurePower = false;
		return ret;
	}

	// Set energy divisor
	unsigned long long divisor;
	if(MSR::readMSR(_msrFD, MSR_RAPL_POWER_UNIT, &divisor))
	{
		_canMeasurePower = false;
		return COULD_NOT_GET_ENERGY_DIVISOR;
	}
	energyMultiplier(ENERGY_UNIT(divisor));

	return SUCCESS;
}

/*
 * Get energy consumed by reading RAPL registers.
 *
 * TODO arguments & retval
 */
retval_t IntelRAPLDevice::_measureRAPLEnergy(unsigned& pkg,
										  unsigned& pp0,
										  unsigned& pp1,
										  unsigned& dram)
{
	unsigned long long reading;

	// Package
	if(MSR::readMSR(_msrFD, MSR_PKG_ENERGY_STATUS, &reading))
		return COULD_NOT_READ_PKG_STATUS;
	pkg = ENERGY_VAL(reading);

	// PP0
	if(MSR::readMSR(_msrFD, MSR_PP0_ENERGY_STATUS, &reading))
		return COULD_NOT_READ_PP0_STATUS;
	pp0 = ENERGY_VAL(reading);

	// PP1
	if(_pp1Enabled)
	{
		if(MSR::readMSR(_msrFD, MSR_PP1_ENERGY_STATUS, &reading))
			return COULD_NOT_READ_PP1_STATUS;
		pp1 = ENERGY_VAL(reading);
		DEBUG("Read pp1: " << pp1);
	}
	else
		pp1 = 0;

	// DRAM
	if(_dramEnabled)
	{
		if(MSR::readMSR(_msrFD, MSR_DRAM_ENERGY_STATUS, &reading))
			return COULD_NOT_READ_DRAM_STATUS;
		dram = ENERGY_VAL(reading);
	}
	else
		dram = 0;

	return SUCCESS;
}

/*
 * Update the accounting information for the specified energy & power using the
 * new & previous RAPL readings.
 *
 * @param newReading the new RAPL energy status reading
 * @param oldReading a reference to the object's previous RAPL energy status reading
 * @param energy a reference to the object's energy accounting info
 * @param power a reference to the object's power accounting info
 */
void IntelRAPLDevice::_updateRAPLAccounting(unsigned newReading, unsigned& oldReading,
										   double& energy, double& power,
										   struct timespec& time)
{
	// Calculate difference in readings
	unsigned consumed;
	if(newReading < oldReading) // Energy reading wrapped around
		consumed = (UINT32_MAX - oldReading) + newReading;
	else
		consumed = newReading - oldReading;
	oldReading = newReading;

	// Energy accounting
	double joules = (double)consumed * _esu;
	energy += joules;

	// Average power over last interval
	double seconds = (double)time.tv_sec + ((double)time.tv_nsec / 1e9);
	power = joules / seconds;
}
