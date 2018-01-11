/*
 * VendorManager.h - standard interface for managing power for different
 * vendors/devices.  Encapsulates all vendor-specific management details.
 *
 *  Created on: Apr 29, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef VENDORMANAGER_H_
#define VENDORMANAGER_H_

#include <string>
#include <vector>
#include <ctime>

#include "common.h"

#include "Device.h"

/* Supported vendors */
enum vendor {
	INTEL = 0,
	AMD,
	NVIDIA,
	NUM_VENDORS
};

/* Human-readable vendor names */
extern const char* vendorNames[];

/* Vendor management libraries (dynamically loaded) */
extern const char* vendorLibraries[];

/*
 * Base class for vendor-specific device power management.
 *
 * To add a new vendor manager, implement a child class that inherits
 * VendorManager.  Then, add the appropriate values to vendor, vendorNames &
 * vendorLibraries -- this signals PowerManager to dynamically check for
 * support.
 */
class VendorManager {
public:
	VendorManager() : _mlHandle(NULL), _numDevices(MAX_DEVICES) {}
	virtual ~VendorManager() {}; // To appease the compiler

	/* Discover (VendorManager only) */
	static bool isAvailable(enum vendor);

	/* Queries */
	virtual enum vendor vendor() const = 0;
	virtual std::string version() const = 0;
	virtual unsigned numDevices() const { return _numDevices; }
	virtual std::string getDeviceInfo(unsigned dev) const = 0;
	virtual const Device* getDevice(unsigned dev) const;

	/* Setters */
	virtual retval_t enableDevice(unsigned dev);
	virtual retval_t disableDevice(unsigned dev);

	/* Actions */
	virtual void resetEnergyMeasurements();
	virtual void startPowerMonitoring() = 0;
	virtual void measurePower(struct timespec& elapsedTime) = 0;
	virtual void stopPowerMonitoring();

protected:
	/* Fields */
	void* _mlHandle; // Library handle
	unsigned _numDevices; // Total number of devices (supported + unsupported)
	std::vector<Device*> _devices; // Managed devices

	/* Functions */
	virtual void _initializeDevices() = 0; // Initialize managed devices

private:
	/* Functions (VendorManager only) */
	static bool _probeLibrary(const char* lib); // Check if loadable
};

#endif /* VENDORMANAGER_H_ */
