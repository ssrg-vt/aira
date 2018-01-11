/*
 * MSR.cpp
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#include <sstream>
#include <string>

// Linux file operations
#include <unistd.h>
#include <sys/stat.h>

#include "MSR.h"

/*
 * Open the specified MSR file.  If successfully opened, place the file
 * descriptor into msrFD reference.
 *
 * @param cpuNum the number of the CPU for which to access MSRs
 * @param msrFD a reference to the memory where the opened file descriptor is
 *              stored
 * @param flags flags used to open the file (default is O_RDONLY)
 * @return SUCCESS if the MSR file was opened, an error code otherwise
 */
retval_t MSR::openMSR(unsigned cpuNum, int& msrFD, int flags)
{
	// Check for MSR device files
	std::stringstream ss;
	ss << "/dev/cpu/" << cpuNum << "/msr";
	std::string fname = ss.str();
	struct stat exists;
	if(stat(fname.c_str(), &exists))
		return MSR_FILE_DOES_NOT_EXIST;

	// Open file
	msrFD = open(fname.c_str(), flags);
	if(msrFD == -1)
		return COULD_NOT_OPEN_MSR;

	return SUCCESS;
}

/*
 * Read the specified MSR.
 *
 * @param msrFD the open MSR file descriptor
 * @param msrReg which MSR register to read
 * @param data a pointer to where the register's contents are stored
 * @return SUCCESS if the register was successfully read, false otherwise
 */
retval_t MSR::readMSR(int msrFD, int msrReg, unsigned long long* data)
{
	ssize_t readSize = pread(msrFD, data, sizeof(unsigned long long), msrReg);
	if(readSize != sizeof(unsigned long long))
		return COULD_NOT_READ_MSR;
	return SUCCESS;
}

/*
 * Close the MSR file.
 *
 * Note: if the file descriptor does not point to a valid file, SUCCESS is
 * returned.
 *
 * @param msrFD the file descriptor to close
 * @return SUCCESS if the file was closed (or if it was never opened), an error
 *         code otherwise
 */
retval_t MSR::closeMSR(int msrFD)
{
	if(msrFD > -1)
	{
		if(close(msrFD))
			return COULD_NOT_CLOSE_MSR;
	}
	return SUCCESS;
}
