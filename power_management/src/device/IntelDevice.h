/*
 * IntelDevice.h
 *
 *  Created on: May 6, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_DEVICE_INTELDEVICE_H_
#define SRC_DEVICE_INTELDEVICE_H_

#include <vector>

#include "Device.h"
#include "Intel.h"

enum IntelDevType {
	INTEL_RAPL = 0,
	XEON_PHI
};

class IntelDevice: public Device {
public:
	IntelDevice(enum IntelDevType devType) : _intelDevType(devType)
	{
		switch(_intelDevType) {
		case INTEL_RAPL:
			_devType = CPU;
			break;
		case XEON_PHI:
			_devType = ACCELERATOR;
			break;
		}
	}
	virtual ~IntelDevice() {}

	// Getters & Setters
	enum IntelDevType intelDevType() const { return _intelDevType; }

protected:
	enum IntelDevType _intelDevType;
};

#endif /* SRC_DEVICE_INTELDEVICE_H_ */
