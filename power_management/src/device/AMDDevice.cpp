/*
 * AMDDevice.cpp
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#include "AMDDevice.h"
#include "AMD.h"
#include "MSR.h"

/*
 * Default constructor
 */
AMDDevice::AMDDevice() : _amdDevType(AMD_CPU), _msrFD(-1)
{
	_devType = CPU;
}

/*
 * Constructor - set device type
 *
 * @param devType the type of the device
 */
AMDDevice::AMDDevice(enum AMDDevType devType)
	: _amdDevType(devType), _msrFD(-1)
{
	switch(_amdDevType)
	{
	case AMD_CPU:
		_devType = CPU;
		break;
	case AMD_GPU:
		_devType = GPU;
		break;
	case AMD_APU:
		_devType = APU;
		break;
	default:
		_devType = UNKNOWN_TYPE;
		break;
	}
}

/*
 * Default destructor - close MSR device file
 */
AMDDevice::~AMDDevice()
{
	MSR::closeMSR(_msrFD);
}

/*
 * Initialize power measurement for the CPU.
 */
retval_t AMDDevice::initializeAMDPower()
{
	assert(_amdDevType == AMD_CPU);

	retval_t ret, ret2;
	ret = MSR::openMSR(_cpus[0], _msrFD);
	if(ret)
	{
		_canMeasurePower = false;
		return ret;
	}

	// Try reading relevant MSRs
	unsigned long long val;
	ret = MSR::readMSR(_msrFD, MSR_PROCESSOR_POWER_IN_TDP, &val);
	ret2 = MSR::readMSR(_msrFD, MSR_POWER_AVERAGING_PERIOD, &val);
	if(ret || ret2)
	{
		_canMeasurePower = false;
		return POWER_MSRS_NOT_AVAILABLE;
	}

	return SUCCESS;
}

