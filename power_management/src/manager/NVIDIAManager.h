/*
 * NVIDIAManager.h - management interface for NVIDIA devices
 *
 *  Created on: Apr 29, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef NVIDIAMANAGER_H_
#define NVIDIAMANAGER_H_

#include "VendorManager.h"
#include "NVIDIA.h"

class NVIDIAManager: public VendorManager {
public:
	NVIDIAManager();
	virtual ~NVIDIAManager();

	/* Queries */
	virtual enum vendor vendor() const { return NVIDIA; }
	virtual std::string version() const;
	virtual std::string getDeviceInfo(unsigned dev) const;

	/* Actions */
	virtual void startPowerMonitoring();
	virtual void measurePower(struct timespec& elapsedTime);

protected:
	void _initializeDevices();

private:
	void _loadFunctions();

	/* Pointers to dynamically-loaded NVML functions */
	nvmlInit _init;
	nvmlShutdown _shutdown;
	nvmlSystemGetDriverVersion _version;
	nvmlDeviceGetCount _getNumDevices;
	nvmlDeviceGetHandleByIndex _getDevice;
	nvmlDeviceGetName _getName;
	nvmlDeviceGetClockInfo _getClock;
	nvmlDeviceGetPowerManagementMode _powerEnabled;
	nvmlDeviceGetPowerState _getPstate;
	nvmlDeviceGetPowerUsage _getPower;
	nvmlDeviceGetTemperature _getTemperature;
};

#endif /* NVIDIAMANAGER_H_ */
