/*
 * Device.cpp
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <sstream>
#include <ctime>

#include "common.h"

#include "Device.h"

/*
 * Default constructor.
 */
Device::Device()
	: _name("N/A"), _clockSpeed(0), _devType(UNKNOWN_TYPE),
	  _canMeasurePower(true), _doPowerMeasurement(true), _pState(-1),
	  _started(false), _power(0), _energy(0) {}

/*
 * Constructor - initialize device name & clock speed.
 *
 * @param name the name of the device
 * @param clockSpeed the clock speed of the device
 */
Device::Device(std::string& name, unsigned clockSpeed)
	: _name(name), _clockSpeed(clockSpeed), _devType(UNKNOWN_TYPE),
	  _canMeasurePower(true), _doPowerMeasurement(true), _pState(-1),
	  _started(false), _power(0), _energy(0) {}

/*
 * Start energy consumption accounting for the device.
 *
 * @return SUCCESS if started accounting, or an error code otherwise
 */
retval_t Device::startEnergyAccounting()
{
	if(_started) return ACCOUNTING_ALREADY_STARTED;
	_started = true;
	clock_gettime(CLOCK_REALTIME, &_start);
	return SUCCESS;
}

/*
 * Stop energy consumption accounting for the device.
 *
 * @return SUCCESS if stopped accounting, or an error code otherwise
 */
retval_t Device::stopEnergyAccounting()
{
	if(!_started) return ACCOUNTING_NOT_STARTED;
	_started = false;
	return SUCCESS;
}

/*
 * Accumulate energy consumption.  Store the specified power and add energy
 * consumed during the elapsed time.
 *
 * @param reading the power reading for the device (in Watts)
 * @param time the elapsed time between power readings
 * @return SUCCESS if added energy consumption, or an error code otherwise
 */
retval_t Device::addEnergyConsumption(unsigned reading, struct timespec& time)
{
	if(!_started) return ACCOUNTING_NOT_STARTED;
	_power = (double)reading;
	_energy += reading * seconds(time);
	return SUCCESS;
}

/*
 * Reset the power & energy consumption counters for the device.
 *
 * @return should always return SUCCESS, even if in the middle of accounting
 */
retval_t Device::resetDeviceExpenditure()
{
	_power = 0;
	_energy = 0;
	return SUCCESS;
}

/*
 * Return a string that lists previous power consumption measurement.
 *
 * @return a string with the last power consumption measurements
 */
std::string Device::power() const
{
	std::stringstream power;
	power << _power << "W";
	return power.str();
}

/*
 * Convert a timespec struct to seconds.
 *
 * @param a timespec struct
 * @return the equivalent number of seconds, in a double
 */
double Device::seconds(struct timespec& ts)
{
	return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
}
