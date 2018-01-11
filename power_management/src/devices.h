/*
 * devices.h
 *
 *  Created on: May 27, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SRC_DEVICES_H_
#define SRC_DEVICES_H_

/* Max number of devices supported by a given vendor */
#define MAX_DEVICES 4096

/* Device types */
typedef enum devtype_t {
	CPU = 0,
	GPU,
	APU,
	ACCELERATOR,

	UNKNOWN_TYPE = 999
} devtype_t;

#endif /* SRC_DEVICES_H_ */
