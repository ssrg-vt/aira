/*
 * AMDManager.h
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#ifndef SRC_MANAGER_AMDMANAGER_H_
#define SRC_MANAGER_AMDMANAGER_H_

#include "VendorManager.h"
#include "AMDDevice.h"

class AMDManager: public VendorManager {
public:
	AMDManager();
	virtual ~AMDManager();
	static bool isAvailable(enum vendor);

	/* Queries */
	virtual enum vendor vendor() const { return AMD; }
	virtual std::string version() const;
	virtual std::string getDeviceInfo(unsigned dev) const;

	/* Actions */
	virtual void startPowerMonitoring();
	virtual void measurePower(struct timespec& elapsedTime);

protected:
	void _initializeDevices();

private:
	void measureAMDCPUPower(AMDDevice* dev, struct timespec& elapsedTime);
	void measureAMDGPUPower(AMDDevice* dev, struct timespec& elapsedTime);
};

#endif /* SRC_MANAGER_AMDMANAGER_H_ */
