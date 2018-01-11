/*
 * Device.h
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef DEVICE_DEVICE_H_
#define DEVICE_DEVICE_H_

#include <string>

#include "common.h"

class Device {
public:
	Device();
	Device(std::string& name, unsigned clockSpeed);
	virtual ~Device() {}

	// Power/energy accounting
	virtual retval_t startEnergyAccounting();
	virtual retval_t stopEnergyAccounting();
	virtual retval_t addEnergyConsumption(unsigned reading, struct timespec& time);
	virtual retval_t resetDeviceExpenditure();

	// Getters
	std::string name() const { return _name; }
	unsigned clockSpeed() const { return _clockSpeed; }
	devtype_t devType() const { return _devType; }
	bool canMeasurePower() const { return _canMeasurePower; }
	bool isMeasurementEnabled() const { return _doPowerMeasurement; }
	bool measurePower() const { return _canMeasurePower && _doPowerMeasurement; }
	unsigned pState() const { return _pState; }
	virtual std::string power() const;
	double avgPower() const { return _power; }
	double energyConsumed() const { return _energy; }

	// Setters
	void setName(std::string& name) { _name = name; }
	void setClockSpeed(unsigned clockSpeed) { _clockSpeed = clockSpeed; }
	void enablePowerMeasurement() { _doPowerMeasurement = true; }
	void disablePowerMeasurement() { _doPowerMeasurement = false; }
	void setPState(unsigned pState) { _pState = pState; }

protected:
	// General information
	std::string _name;
	unsigned _clockSpeed; // In MHz
	devtype_t _devType;
	bool _canMeasurePower; // Are we able to measure power?
	bool _doPowerMeasurement; // Are we going to measure this device's power?
	int _pState; // Vendor/device-specific
	struct timespec _start;

	// Energy expenditure state
	bool _started;
	double _power; // In Watts
	double _energy; // In Joules

	// Functions
	static double seconds(struct timespec& ts);
};

#endif /* DEVICE_DEVICE_H_ */
