/*
 * MSR.h
 *
 *  Created on: May 8, 2015
 *      Author: rlyerly
 */

#ifndef SRC_UTILITY_MSR_H_
#define SRC_UTILITY_MSR_H_

#include <fcntl.h>

#include "common.h"

/*
 * Model-specific registers (MSR) support.  These functions are CPU-agnostic.
 */
namespace MSR {

retval_t openMSR(unsigned cpuNum, int& msrFD, int flags = O_RDONLY);
retval_t readMSR(int msrFD, int msrReg, unsigned long long* data);
retval_t closeMSR(int msrFD);

}

#endif /* SRC_UTILITY_MSR_H_ */
