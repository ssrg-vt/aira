/*
 * main.cpp - entry point for power management.
 *
 *  Created on: Apr 29, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <vector>
#include <cstdlib>
#include <unistd.h>

#include "common.h"

/* Vendor-specific managers */
#include "VendorManager.h"
#include "IntelManager.h"
#include "AMDManager.h"
#include "NVIDIAManager.h"

/* Daemon configuration */
#include "daemon.h"

using namespace std;

/* Main configuration */
bool listDevices = false;
bool daemonize = false;

/* Print usage & exit */
void printHelp()
{
	cout << "PowerManager: record & manage system power polices" << endl
		<< "Usage: ./PowerManager [ OPTIONS ]" << endl << endl
		<< "Options:" << endl
		<< "    -h          : print help & exit" << endl
		<< "    -l          : list device info" << endl
		<< "    -m <period> : start power monitoring daemon, sampling w/ specified period in ms (default is " << period << ")" << endl
		<< "    -f <file>   : log daemon output to the specified file (default is " << logFile << ")" << endl
		<< "    -v          : enable verbose logging (dumps all power measurements, increases per-measurement time)" << endl;
	exit(0);
}

/* Parse command-line args */
void parseArgs(int argc, char** argv)
{
	int c;
	while((c = getopt(argc, argv, "hlvd:")) != -1)
	{
		switch(c) {
		case 'h':
			printHelp();
			break;
		case 'd':
			daemonize = true;
			period = atoi(optarg);
			break;
		case 'l':
			listDevices = true;
			break;
		case 'v':
			verboseLogging = true;
			break;
		default:
			cout << "Unknown argument \"" << (char)c << "\"" << endl;
			printHelp();
			break;
		}
	}
}

/* Manager entry point */
int main(int argc, char** argv)
{
	parseArgs(argc, argv);

	// Load vendor-specific power managers
	DEBUG("Initializing vendor-specific managers");
	vector<VendorManager*> managers;
	for(int v = 0; v < NUM_VENDORS; v++)
	{
		DEBUG("Checking for " << vendorNames[v] << " support");
		if(VendorManager::isAvailable((enum vendor)v))
		{
			switch(v) {
			case INTEL:
				managers.push_back(new IntelManager());
				break;
			case AMD:
				managers.push_back(new AMDManager());
				break;
			case NVIDIA:
				managers.push_back(new NVIDIAManager());
				break;
			}
		}
		else
			DEBUG("  not available");
	}

	if(listDevices)
	{
		INFO("Vendor information:");
		for(size_t i = 0; i < managers.size(); i++)
		{
			string vendor(vendorNames[managers[i]->vendor()]);
			string version = managers[i]->version();
			unsigned numDevices = managers[i]->numDevices();
			INFO("  " << vendor << ", version " << version << ", found " << numDevices << " device(s)");

			for(size_t j = 0; j < managers[i]->numDevices(); j++)
			{
				if(managers[i]->getDevice(j)->canMeasurePower())
					INFO("    " << managers[i]->getDeviceInfo(j));
				else
					INFO("    " << managers[i]->getDeviceInfo(j) << " (NOT SUPPORTED)");
			}
		}
	}

	if(daemonize)
	{
		pid_t child = 0;
		DEBUG_NOEOL("Starting power monitoring daemon (period = " << period << "ms)...");
		retval_t ret = startDaemon(managers, period, child);
		if(ret == SUCCESS)
			DEBUG_CONT("daemon PID = " << child << endl);
		else
		{
			DEBUG_CONT("error: " << retvalStr[ret] << endl);
			return COULD_NOT_START;
		}
	}

	return SUCCESS;
}
