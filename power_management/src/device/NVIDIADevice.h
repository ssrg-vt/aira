/*
 * NVIDIADevice.h
 *
 *  Created on: May 1, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_DEVICE_NVIDIADEVICE_H_
#define SRC_DEVICE_NVIDIADEVICE_H_

#include "Device.h"
#include "NVIDIA.h"

class NVIDIADevice : public Device {
public:
	NVIDIADevice(nvmlDevice_t dev);
	virtual ~NVIDIADevice();

	/* Getters */
	nvmlDevice_t getDev() { return _dev; } // Should we return a const value?

	/*
	 * Setters
	 *
	 * Needed because NVML logic is contained in NVIDIAManager, not NVIDIADevice
	 */
	void canMeasurePower(bool canMeasurePower) { _canMeasurePower = canMeasurePower; }

private:
	nvmlDevice_t _dev;
};

#endif /* SRC_DEVICE_NVIDIADEVICE_H_ */
