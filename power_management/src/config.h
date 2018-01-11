/*
 * config.h - configuration for power management.
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/* Macro stringification */
#define STR( x ) #x
#define XSTR( x ) STR(x)

/* Version information */
#define MAJOR 0
#define MINOR 2
#define VERSION_STR XSTR(MAJOR) "." XSTR(MINOR)

/*
 * Enable/disable debug messages.
 */
//#define _VERBOSE 1

#endif /* CONFIG_H_ */
