/*
 * AMDDevice.h
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#ifndef SRC_DEVICE_AMDDEVICE_H_
#define SRC_DEVICE_AMDDEVICE_H_

#include <vector>

#include "Device.h"
#include "AMD.h"

enum AMDDevType {
	AMD_CPU = 0,
	AMD_GPU,
	AMD_APU
};

class AMDDevice: public Device {
public:
	AMDDevice();
	AMDDevice(enum AMDDevType devType);
	virtual ~AMDDevice();

	/* Power/energy accounting */
	//retval_t addEnergyConsumption(unsigned reading, struct timespec& time);

	/* Getters */
	enum AMDDevType amdDevType() const { return _amdDevType; }
	unsigned numCPUs() const { return _cpus.size(); }
	int msrFD() const { return _msrFD; }

	/* Setters */
	void addCPU(int cpuNum) { _cpus.push_back(cpuNum); }

	/* AMD power initialization */
	retval_t initializeAMDPower();

private:
	enum AMDDevType _amdDevType;
	std::vector<int> _cpus;
	int _msrFD;
};

#endif /* SRC_DEVICE_AMDDEVICE_H_ */
