/*
 * IntelManager.h
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef LINUXCPUMANAGER_H_
#define LINUXCPUMANAGER_H_

#include "VendorManager.h"
#include "IntelDevice.h"
#include "IntelRAPLDevice.h"

class IntelManager : public VendorManager {
public:
	IntelManager();
	virtual ~IntelManager();
	static bool isAvailable(enum vendor);

	/* Queries */
	virtual enum vendor vendor() const { return INTEL; }
	virtual std::string version() const;
	virtual std::string getDeviceInfo(unsigned dev) const;

	/* Actions */
	virtual void startPowerMonitoring();
	virtual void measurePower(struct timespec& elapsedTime);

protected:
	void _initializeDevices();

private:
	/* Xeon Phi Device Actions */
	// TODO
	unsigned _measureXeonPhiPower(Device* dev);
};

#endif /* LINUXCPUMANAGER_H_ */
