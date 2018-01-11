/*
 * PowerMeasurement.cpp
 *
 *  Created on: May 13, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <csignal>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <pthread.h>

#include "common.h"

#include "VendorManager.h"
#include "IntelManager.h"
#include "AMDManager.h"
#include "NVIDIAManager.h"

#include "IntelDevice.h"
#include "AMDDevice.h"

#include "PowerMeasurement.h"

///////////////////////////////////////////////////////////////////////////////
// Library-private API
///////////////////////////////////////////////////////////////////////////////

static void __attribute__((constructor)) initialize();
static std::vector<VendorManager*> initializeManagers();
static void* alarmThreadLoop(void* handle);

///////////////////////////////////////////////////////////////////////////////
// Configuration, definitions & library data
///////////////////////////////////////////////////////////////////////////////

#define CLOCK CLOCK_MONOTONIC
#define ALARM_SIGNAL SIGALRM

// Thread status/phase
enum threadState {
	// Thread states
	INITIALIZING = 0,
	WAITING_TO_START,
	MONITORING,
	CLEANING_UP,

	// Thread error codes
	SIGNAL_SETUP_ERR,
	EVENT_SETUP_ERR,
	MONITORING_SETUP_ERR,
	MONITORING_CLEANUP_ERR,
	SIGNAL_CLEANUP_ERR,
	EVENT_CLEANUP_ERR
};

struct _powerlib_t {
	// Manager/device data
	std::vector<VendorManager*> managers;

	// Threading data
	pthread_t alarmThread;
	pthread_barrier_t barrier;
	pid_t alarmThreadPID;
	volatile bool keepLooping;
	volatile enum threadState tstate;

	// Timer data
	struct sigaction act;
	struct sigaction oldAct;
	struct sigevent sevp; // Alarm event handling
	timer_t timerid;
	struct itimerspec timerspec;

	// Monitoring data
	volatile bool startedMonitoring;
	volatile bool keepMonitoring;
	int numPeriodsMonitored;
	std::string summary;
};

// Only used for querying information
static std::vector<VendorManager*> queryManagers = initializeManagers();

///////////////////////////////////////////////////////////////////////////////
// Library-private functions
///////////////////////////////////////////////////////////////////////////////

/*
 * Library constructor.
 *
 * Block alarm signal for process (i.e. main thread & children).  An alarm
 * thread will handle all signals for alarm signal.
 */
static void initialize()
{
	sigset_t sig;
	sigemptyset(&sig);
	sigaddset(&sig, ALARM_SIGNAL);
	sigprocmask(SIG_BLOCK, &sig, NULL);
}

/*
 * Initializes a vector of vendor managers.
 *
 * @return a vector of VendorManager* which represent all supported vendors for
 *         the system
 */
static std::vector<VendorManager*> initializeManagers()
{
	std::vector<VendorManager*> initManagers;
	for(int v = 0; v < NUM_VENDORS; v++)
	{
		if(VendorManager::isAvailable((enum vendor)v))
		{
			switch(v) {
			case INTEL:
				initManagers.push_back(new IntelManager());
				break;
			case AMD:
				initManagers.push_back(new AMDManager());
				break;
			case NVIDIA:
				initManagers.push_back(new NVIDIAManager());
				break;
			}
		}
	}
	return initManagers;
}

/*
 * Signal handler for the alarm.
 *
 * @param sig always ALARM_SIGNAL
 * @param siginfo signal information
 * @param context signal context
 */
static void alarmHandler(int sig, siginfo_t* siginfo, void* context)
{
	powerlib_t handle = (powerlib_t)siginfo->si_value.sival_ptr;
	if(siginfo->si_code == SI_TIMER && handle->keepMonitoring)
	{
		handle->numPeriodsMonitored++;
		for(size_t v = 0; v < handle->managers.size(); v++)
			handle->managers[v]->measurePower(handle->timerspec.it_interval);
	}
}

/*
 * Main for thread handling alarms.  Thread is responsible for setting up
 * alarm (including signal handling & event dispatching) and looping until
 * it is instructed to begin power monitoring, or to exit.
 *
 * @param args powerlib_t handle
 * @return NULL, always
 */
static void* alarmThreadLoop(void* args)
{
	powerlib_t handle = (powerlib_t)args;
	handle->alarmThreadPID = getpid();

	// Unblock & set signal handling for alarm signal
	sigset_t sig;
	sigemptyset(&sig);
	sigaddset(&sig, ALARM_SIGNAL);
	sigprocmask(SIG_UNBLOCK, &sig, NULL);
	handle->act.sa_sigaction = alarmHandler;
	sigemptyset(&handle->act.sa_mask);
	handle->act.sa_flags = SA_SIGINFO;
	handle->act.sa_restorer = NULL;
	if(sigaction(ALARM_SIGNAL, &handle->act, &handle->oldAct))
	{
		perror("Could not register handler for alarm");
		handle->tstate = SIGNAL_SETUP_ERR;
		pthread_barrier_wait(&handle->barrier);
		return NULL;
	}

	// Set up alarm delivery
	handle->sevp.sigev_notify = SIGEV_SIGNAL;
	handle->sevp.sigev_signo = ALARM_SIGNAL;
	handle->sevp.sigev_value.sival_ptr = handle;
	if(timer_create(CLOCK, &handle->sevp, &handle->timerid))
	{
		perror("Could not set up event handling for alarm");
		sigaction(ALARM_SIGNAL, &handle->oldAct, NULL);
		handle->tstate = EVENT_SETUP_ERR;
		pthread_barrier_wait(&handle->barrier);
		return NULL;
	}

	handle->startedMonitoring = false;
	handle->numPeriodsMonitored = 0;
	handle->tstate = WAITING_TO_START;
	pthread_barrier_wait(&handle->barrier);

	// Main control loop
	while(true)
	{
		// Wait at the barrier until we should start monitoring or exit
		pthread_barrier_wait(&handle->barrier);
		if(!handle->keepLooping)
		{
			handle->tstate = CLEANING_UP;
			break;
		}

		// Begin power monitoring
		handle->startedMonitoring = true;
		handle->numPeriodsMonitored = 0;
		for(size_t v = 0; v < handle->managers.size(); v++)
		{
			handle->managers[v]->resetEnergyMeasurements();
			handle->managers[v]->startPowerMonitoring();
		}

		if(timer_settime(handle->timerid, 0, &handle->timerspec, NULL))
		{
			handle->tstate = MONITORING_SETUP_ERR;
			pthread_barrier_wait(&handle->barrier);
			break;
		}
		handle->tstate = MONITORING;
		pthread_barrier_wait(&handle->barrier);

		// Signal loop
		while(handle->keepMonitoring)
			pause();

		// Stop power monitoring
		handle->startedMonitoring = false;
		for(size_t v = 0; v < handle->managers.size(); v++)
			handle->managers[v]->stopPowerMonitoring();

		handle->timerspec.it_value.tv_sec = 0;
		handle->timerspec.it_value.tv_nsec = 0;
		handle->timerspec.it_interval.tv_sec = 0;
		handle->timerspec.it_interval.tv_nsec = 0;
		if(timer_settime(handle->timerid, 0, &handle->timerspec, NULL))
		{
			handle->tstate = MONITORING_CLEANUP_ERR;
			pthread_barrier_wait(&handle->barrier);
			break;
		}
		handle->tstate = WAITING_TO_START;
		pthread_barrier_wait(&handle->barrier);
	}

	// Clean up signal handling & timer
	if(sigaction(ALARM_SIGNAL, &handle->oldAct, NULL))
		handle->tstate = SIGNAL_CLEANUP_ERR;
	if(timer_delete(handle->timerid))
		handle->tstate = EVENT_CLEANUP_ERR;

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// General platform information
///////////////////////////////////////////////////////////////////////////////

size_t powerlib_num_managers()
{
	return queryManagers.size();
}

size_t powerlib_num_devices(size_t manager)
{
	assert(manager < queryManagers.size());
	return queryManagers[manager]->numDevices();
}

const char* powerlib_manager_info(size_t manager)
{
	assert(manager < queryManagers.size());
	return vendorNames[queryManagers[manager]->vendor()];
}

const char* powerlib_device_info(size_t manager, size_t device)
{
	assert(manager < queryManagers.size());
	assert(device < queryManagers[manager]->numDevices());
	return queryManagers[manager]->getDeviceInfo(device).c_str();
}

devtype_t powerlib_device_type(size_t manager, size_t device)
{
	assert(manager < queryManagers.size());
	assert(device < queryManagers[manager]->numDevices());
	return queryManagers[manager]->getDevice(device)->devType();
}

int powerlib_device_supported(size_t manager, size_t device)
{
	assert(manager < queryManagers.size());
	assert(device < queryManagers[manager]->numDevices());
	return queryManagers[manager]->getDevice(device)->canMeasurePower();
}

///////////////////////////////////////////////////////////////////////////////
// Monitoring API
///////////////////////////////////////////////////////////////////////////////

powerlib_t powerlib_initialize()
{
	powerlib_t newHandle = new struct _powerlib_t;
	assert(newHandle);

	newHandle->keepLooping = true;
	newHandle->keepMonitoring = false;
	newHandle->managers = initializeManagers();
	powerlib_remove_all_devices(newHandle);

	// Create barrier & alarm thread
	if(pthread_barrier_init(&newHandle->barrier, NULL, 2))
	{
		delete newHandle;
		return NULL;
	}

	if(pthread_create(&newHandle->alarmThread, NULL, alarmThreadLoop, newHandle))
	{
		pthread_barrier_destroy(&newHandle->barrier);
		delete newHandle;
		return NULL;
	}

	// Wait for thread to get set up & check for errors
	pthread_barrier_wait(&newHandle->barrier);
	if(newHandle->tstate != WAITING_TO_START)
	{
		pthread_join(newHandle->alarmThread, NULL);
		pthread_barrier_destroy(&newHandle->barrier);
		delete newHandle;
		return NULL;
	}

	return newHandle;
}

int powerlib_shutdown(powerlib_t handle)
{
	if(!handle)
		return true;

	handle->keepLooping = false;
	pthread_barrier_wait(&handle->barrier);
	pthread_join(handle->alarmThread, NULL);
	pthread_barrier_destroy(&handle->barrier);

	bool retval = (handle->tstate == CLEANING_UP);
	delete handle;
	return retval;
}

int powerlib_start_monitoring(powerlib_t handle, struct timespec* period)
{
	if(!handle)
		return true;
	if(handle->startedMonitoring)
		return true;

	handle->timerspec.it_value = *period;
	handle->timerspec.it_interval = *period;
	handle->keepMonitoring = true;
	pthread_barrier_wait(&handle->barrier);
	pthread_barrier_wait(&handle->barrier); // See if monitoring started
	if(handle->tstate != MONITORING)
		return true;
	else
		return false;
}

int powerlib_stop_monitoring(powerlib_t handle)
{
	if(!handle)
		return true;
	if(!handle->startedMonitoring)
		return true;

	handle->keepMonitoring = false;
	if(raise(ALARM_SIGNAL))
		return true;

	pthread_barrier_wait(&handle->barrier);
	if(handle->tstate != WAITING_TO_START)
		return true;
	else
		return false;
}

int powerlib_add_device(powerlib_t handle, size_t manager, size_t device)
{
	if(!handle)
		return -1;
	if(manager >= handle->managers.size() || device >= handle->managers[manager]->numDevices())
		return -1;

	retval_t ret = handle->managers[manager]->enableDevice(device);
	switch(ret) {
	case SUCCESS:
		return 1;
	case ALREADY_ENABLED:
	case DEV_CANNOT_MEASURE_POWER:
		return 0;
	default:
		return -1;
	}
}

int powerlib_add_devtype(powerlib_t handle, devtype_t type)
{
	if(!handle)
		return -1;

	int numAdded = 0;
	for(size_t i = 0; i < handle->managers.size(); i++)
	{
		for(size_t j = 0; j < handle->managers[i]->numDevices(); j++)
		{
			if(powerlib_device_type(i, j) == type)
			{
				// Reasons this could fail:
				// 1. Device has already been added
				// 2. Device doesn't support power measurement
				// 3. Something bad happened...
				int tmpAdded = powerlib_add_device(handle, i, j);
				if(tmpAdded == -1) return -1;
				else numAdded += tmpAdded;
			}
		}
	}
	return numAdded;
}

int powerlib_add_manager(powerlib_t handle, size_t manager)
{
	if(!handle)
		return -1;
	if(manager >= handle->managers.size())
		return -1;

	int numAdded = 0;
	for(size_t i = 0; i < handle->managers[manager]->numDevices(); i++)
	{
		// Reasons this could fail:
		// 1. Device has already been added
		// 2. Device doesn't support power measurement
		// 3. Something bad happened...
		int tmpAdded = powerlib_add_device(handle, manager, i);
		if(tmpAdded == -1) return -1; // Something bad happened...
		else numAdded += tmpAdded; // Normal operation
	}
	return numAdded;
}

int powerlib_add_all_devices(powerlib_t handle)
{
	if(!handle)
		return -1;

	int numAdded = 0;
	for(size_t i = 0; i < handle->managers.size(); i++)
	{
		int tmpAdded = powerlib_add_manager(handle, i);
		if(tmpAdded == -1) return -1;
		else numAdded += tmpAdded;
	}
	return numAdded;
}

int powerlib_remove_device(powerlib_t handle, size_t manager, size_t device)
{
	if(!handle)
		return -1;
	if(manager >= handle->managers.size() || device >= handle->managers[manager]->numDevices())
		return -1;

	retval_t ret = handle->managers[manager]->disableDevice(device);
	switch(ret) {
	case SUCCESS:
		return 1;
	case ALREADY_DISABLED:
		return 0;
	default:
		return -1;
	}
}

int powerlib_remove_devtype(powerlib_t handle, devtype_t type)
{
	if(!handle)
		return -1;

	int numRemoved = 0;
	for(size_t i = 0; i < handle->managers.size(); i++)
	{
		for(size_t j = 0; j < handle->managers[i]->numDevices(); j++)
		{
			if(powerlib_device_type(i, j) == type)
			{
				// Reasons this could fail:
				// 1. Device has already been removed
				// 3. Something bad happened...
				int tmpRemoved = powerlib_remove_device(handle, i, j);
				if(tmpRemoved == -1) return -1;
				else numRemoved += tmpRemoved;
			}
		}
	}
	return numRemoved;
}

int powerlib_remove_manager(powerlib_t handle, size_t manager)
{
	if(!handle)
		return -1;
	if(manager >= handle->managers.size())
		return -1;

	int numRemoved = 0;
	for(size_t i = 0; i < handle->managers[manager]->numDevices(); i++)
	{
		// Reasons this could fail:
		// 1. Device has already been removed
		// 2. Something bad happened...
		int tmpRemoved = powerlib_remove_device(handle, manager, i);
		if(tmpRemoved == -1) return -1; // Something bad happened...
		else numRemoved += tmpRemoved; // Normal operation
	}
	return numRemoved;
}

int powerlib_remove_all_devices(powerlib_t handle)
{
	if(!handle)
		return -1;

	int numRemoved = 0;
	for(size_t i = 0; i < handle->managers.size(); i++)
	{
		int tmpRemoved = powerlib_remove_manager(handle, i);
		if(tmpRemoved == -1) return -1;
		else numRemoved += tmpRemoved;
	}
	return numRemoved;
}

int powerlib_device_added(powerlib_t handle, size_t manager, size_t device)
{
	if(manager >= handle->managers.size() ||
	   device >= handle->managers[manager]->numDevices())
		return -1;

	return handle->managers[manager]->getDevice(device)->measurePower();
}

const char* powerlib_summarize_measurements(powerlib_t handle)
{
	if(!handle)
		return NULL;

	std::stringstream ss;
	ss << "***********\n* Summary *\n***********\nNumber of periods measured: "
		<< handle->numPeriodsMonitored << "\n";
	for(size_t v = 0; v < handle->managers.size(); v++)
	{
		for(size_t d = 0; d < handle->managers[v]->numDevices(); d++)
		{
			if(handle->managers[v]->getDevice(d)->measurePower())
				ss << "  " << handle->managers[v]->getDevice(d)->name()
					<< ": " << powerlib_energy(handle, v, d) << "J, "
					<< powerlib_avg_power(handle, v, d) << "W" << std::endl;
		}
	}

	handle->summary = ss.str();
	return handle->summary.c_str();
}

int powerlib_num_periods_measured(powerlib_t handle)
{
	if(!handle)
		return -1;
	return handle->numPeriodsMonitored;
}

double powerlib_energy(powerlib_t handle, size_t manager, size_t device)
{
	if(!handle)
		return 0.0;
	assert(manager < handle->managers.size() && device < handle->managers[manager]->numDevices());
	return handle->managers[manager]->getDevice(device)->energyConsumed();
}

double powerlib_avg_power(powerlib_t handle, size_t manager, size_t device)
{
	if(!handle)
		return 0.0;
	assert(manager < handle->managers.size() && device < handle->managers[manager]->numDevices());
	return handle->managers[manager]->getDevice(device)->avgPower();
}

