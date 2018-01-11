/*
 * daemon.h
 *
 *  Created on: Apr 30, 2015
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef DAEMON_H_
#define DAEMON_H_

/* Configuration */
extern const char* pidFile;
extern const char* logFile;
extern unsigned period;
extern bool verboseLogging;

/* Fork daemon process */
retval_t startDaemon(std::vector<VendorManager*> managers, unsigned period, pid_t& child);

#endif /* DAEMON_H_ */
