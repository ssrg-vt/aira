/*
 * NVIDIADevice.cpp
 *
 *  Created on: May 1, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include "NVIDIADevice.h"

NVIDIADevice::NVIDIADevice(nvmlDevice_t dev) : _dev(dev)
{
	_devType = GPU;
}

NVIDIADevice::~NVIDIADevice()
{
	// nothing for now...
}

