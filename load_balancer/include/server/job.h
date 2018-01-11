/*
 * Job declaration.  Encapsulates a pending job for the server, including all
 * information required to apply analyses and make policy decisions.
 *
 * Author: Rob Lyerly
 * Date: 9/25/2015
 */

#ifndef _JOB_H
#define _JOB_H

/* Timespec/unsigned long conversions */
#define toNS( ts ) ((ts.tv_sec * 1000000000) + ts.tv_nsec)

class Job
{
public:
	/* State.  Kept public for ease-of-use. */
	int fd;       /* Socket for communication w/ client */
	pid_t client; /* Client PID */

	struct timespec queued; /* Time kernel is enqueued waiting for device. */
	struct timespec start;  /* Kernel start time (does not include queue time) */
	struct timespec end;    /* Kernel completion time */

	struct kernel_features features; /* Kernel features for model evaluation */
	struct resource_alloc alloc;     /* Resources allocated */

	std::vector<float> predictions; /* Predictions for each architecture */

	/* API */
	Job(struct connection conn);
	unsigned long queuedTime() const { return toNS(queued); };
	unsigned long startTime() const { return toNS(start); };
	unsigned long endTime() const { return toNS(end); };
};

#endif /* _JOB_H */

