/*
 * daemon.cpp
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include <csignal>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

#include "VendorManager.h"

/* Configuration */
const char* pidFile = "/var/run/PowerManager_Monitoring.pid";
const char* logFile = "/var/run/PowerManager_Monitoring.txt";
unsigned period = 16; // in ms
bool verboseLogging = false;

/* Signals */
const int exitSig = SIGUSR1;
volatile bool keepLooping = true;

/*
 * Signal handler for stopping the daemon.
 *
 * @param signum signal triggering the handler
 */
void exitHandler(int signum)
{
	keepLooping = false;
}

/*
 * Log information to file.  Prepends time to logging message.
 *
 * @param fstream output file stream for which to write
 * @param msg string containing a message to write
 * @param extraNewLine if true, write an extra newline to the log
 */
void log(std::ofstream& fstream, std::string& msg, bool extraNewline = false)
{
	assert(fstream.is_open());
	time_t t = time(0);
	struct tm* now = localtime(&t);
	std::stringstream ts;
	ts << now->tm_hour << ":" << now->tm_min << "." << now->tm_sec;

	fstream << "[" << ts.str() << "] " << msg << std::endl;
	if(extraNewline) fstream << "[" << ts.str() << "]" << std::endl;
}

/*
 * Register signal handlers so that outside processes can communicate with the
 * daemon.
 *
 * @return SUCCESS if signals were registered, COULD_NOT_REGISTER_SIGNALS otherwise
 */
retval_t registerSignals()
{
	struct sigaction exitAct;
	exitAct.sa_handler = exitHandler;
	if(sigaction(exitSig, &exitAct, NULL)) return COULD_NOT_REGISTER_SIGNALS;
	return SUCCESS;
}

/*
 * Write PID to file so that outside processes know with whom to communicate.
 *
 * @param fname file for which to write PID
 * @return SUCCESS if PID was written to fname, COULD_NOT_WRITE_PID otherwise
 */
retval_t writePID(const char* fname)
{
	std::ofstream pidFile;
	pidFile.open(fname);
	if(!pidFile.is_open()) return COULD_NOT_WRITE_PID;
	pidFile << getpid() << std::endl;
	pidFile.close();
	return SUCCESS;
}

/*
 * Start logging.  Open log file & print log header.
 *
 * @param fname file for which to open for logging
 * @param fstream stream which is opened for writing to fname
 * @param managers device managers used for monitoring (writes log information about managers)
 * @return SUCCESS if logging was started, an error code otherwise
 */
retval_t startLogging(const char* fname, std::ofstream& fstream, std::vector<VendorManager*>& managers)
{
	fstream.open(fname);
	if(!fstream.is_open()) return COULD_NOT_OPEN_LOGFILE;

	time_t t = time(0);
	struct tm* now = localtime(&t);
	std::stringstream ss;

	std::string text("PowerManager, v" VERSION_STR);
	log(fstream, text);
	ss << now->tm_mon << "/" << now->tm_mday << "/" << (now->tm_year + 1900);
	text = ss.str();
	log(fstream, text);
	text = "Started logging";
	log(fstream, text, true);

	text = "Monitoring power for the following device(s)";
	log(fstream, text);
	for(unsigned i = 0; i < managers.size(); i++)
	{
		text = vendorNames[managers[i]->vendor()];
		log(fstream, text);

		if(managers[i]->numDevices() == 0)
		{
			text = "  (n/a)";
			log(fstream, text);
		}
		else
		{
			for(unsigned j = 0; j < managers[i]->numDevices(); j++)
			{
				if(managers[i]->getDevice(j)->measurePower())
				{
					text = "  " + managers[i]->getDeviceInfo(j);
					log(fstream, text);
				}
			}
		}
	}
	text = "";
	log(fstream, text);

	// Go ahead and flush to get the file created
	fstream.flush();

	// Redirect stdout & stderr to logfile (for debugging purposes)
	// TODO do we need freopen calls?
	if(!freopen(fname, "a", stdout)) return COULD_NOT_REDIRECT_STD_STREAMS;
	if(!freopen(fname, "a", stderr)) return COULD_NOT_REDIRECT_STD_STREAMS;
	std::cout.rdbuf(fstream.rdbuf());
	std::cerr.rdbuf(fstream.rdbuf());

	return SUCCESS;
}

/*
 * Log instantaneous (i.e. per period) power to file.  Only performed if
 * verbose logging is enabled at command line.
 *
 * @param fstream output file stream for which to write power measurements
 * @param managers managers for which to probe power
 */
void logPower(std::ofstream& fstream, std::vector<VendorManager*>& managers)
{
	const Device* dev;
	std::string text("Instantaneous power:");
	log(fstream, text);

	for(unsigned i = 0; i < managers.size(); i++)
	{
		for(unsigned j = 0; j < managers[i]->numDevices(); j++)
		{
			if(managers[i]->getDevice(j)->measurePower())
			{
				std::stringstream ss;
				dev = managers[i]->getDevice(j);
				ss << "  " << dev->name() << ": " << dev->power();
				text = ss.str();
				log(fstream, text);
			}
		}
	}
	text = "";
	log(fstream, text);
}

/*
 * Summarize total per-device energy consumed when the daemon has been told to
 * quit.
 *
 * @param output file stream for which to write energy consumption data
 * @param managers managers for which to get energy consumption data
 */
void summarizeEnergyConsumption(std::ofstream& fstream, std::vector<VendorManager*> managers)
{
	const Device* dev;
	std::string text("--------------------------");
	log(fstream, text);
	text = "Energy consumption summary";
	log(fstream, text);
	text = "--------------------------";
	log(fstream, text, true);

	for(unsigned i = 0; i < managers.size(); i++)
	{
		if(managers[i]->numDevices() > 0)
		{
			text = vendorNames[managers[i]->vendor()];
			log(fstream, text);

			for(unsigned j = 0; j < managers[i]->numDevices(); j++)
			{
				if(managers[i]->getDevice(j)->measurePower())
				{
					std::stringstream ss;
					dev = managers[i]->getDevice(j);
					ss << "  " << dev->name() << ": " << dev->energyConsumed() << "J";
					text = ss.str();
					log(fstream, text);
				}
			}
		}
	}
	text = "";
	log(fstream, text);
}

/*
 * Cleanup daemon execution, including removing any created files.
 *
 * @param fname PID file to be removed
 * @param fstream log file stream to be closed
 */
retval_t cleanup(const char* fname, std::ofstream& fstream)
{
	std::string text("Stopped logging");
	log(fstream, text);
	fstream.close(); // We don't care if the log file couldn't be closed
	if(std::remove(fname)) return COULD_NOT_DELETE_PID;
	//TODO destroy managers list?
	return SUCCESS;
}

/*
 * If nanosleep was interrupted, calculate how much time we actually slept &
 * write it into the remainder struct.
 *
 * @param req the requested amount of time to sleep
 * @param rem the amount of time remaining (the amount of time we didn't sleep)
 */
void getSleepTime(struct timespec& req, struct timespec& rem)
{
	unsigned long long nsReq = (req.tv_sec * 1000000000) + req.tv_nsec;
	unsigned long long nsRem = (rem.tv_sec * 1000000000) + rem.tv_nsec;
	nsRem = nsReq - nsRem;
	rem.tv_sec = nsRem / 1000000000;
	rem.tv_nsec = nsRem % 1000000000;
}

/*
 * Daemon main.  Does not return!
 *
 * @param managers a vector of VendorManagers, which are used to, ya know,
 *                 manage devices
 * @param period the period for monitoring power/energy consumption (in ms)
 */
void daemonMain(std::vector<VendorManager*> managers, unsigned period)
{
	std::ofstream logStream;
	struct timespec sleep, rem;
	sleep.tv_sec = period / 1000;
	sleep.tv_nsec = (period % 1000) * 1000000;

	// Initialize
	if(registerSignals()) exit(COULD_NOT_REGISTER_SIGNALS);
	if(writePID(pidFile)) exit(COULD_NOT_WRITE_PID);
	if(startLogging(logFile, logStream, managers)) exit(COULD_NOT_OPEN_LOGFILE);
	for(unsigned i = 0; i < managers.size(); i++)
	{
		managers[i]->resetEnergyMeasurements();
		managers[i]->startPowerMonitoring();
	}

	// Power monitoring loop - continue until receiving exit signal
	// TODO subtract time spent measuring power from sleep time for next period
	while(keepLooping)
	{
		if(!nanosleep(&sleep, &rem)) // Slept successfully
			for(unsigned i = 0; i < managers.size(); i++)
				managers[i]->measurePower(sleep);
		else // Interrupted
		{
			getSleepTime(sleep, rem);
			for(unsigned i = 0; i < managers.size(); i++)
				managers[i]->measurePower(rem);
		}

		if(verboseLogging)
			logPower(logStream, managers);
	}

	// Clean up
	for(unsigned i = 0; i < managers.size(); i++)
		managers[i]->stopPowerMonitoring();
	summarizeEnergyConsumption(logStream, managers);
	exit(cleanup(pidFile, logStream));
}

/*
 * Fork daemon.  Parent returns to main, child executes daemon loop...
 * for-ev-er (see the Sandlot).
 *
 * @param managers list of device managers used to monitor device power
 * @param period the period (in ms) for which to monitor power
 * @param child the forked child's PID
 * @return SUCCESS if daemon was started, an error code otherwise
 */
retval_t startDaemon(std::vector<VendorManager*> managers, unsigned period, pid_t& child)
{
	// Make sure we're root
	if(geteuid()) return NOT_ROOT;

	// Fork the child
	pid_t pid, sid;
	pid = fork();

	if(pid < 0) return COULD_NOT_FORK; // Couldn't fork child

	// Parent returns
	if(pid > 0)
	{
		child = pid;
		return SUCCESS;
	}

	/* Child/daemon initialization */
	// Clear child's file mask
	umask(0);

	// Create new process group
	sid = setsid();
	if(sid < 0) exit(COULD_NOT_CREATE_PGROUP);

	// Change working directory to root
	if((chdir("/")) < 0) exit(COULD_NOT_CHANGE_WD);

	// Close standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Execute daemon (doesn't return!)
	daemonMain(managers, period);
	return SUCCESS; // To appease compiler
}
