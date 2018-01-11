#ifndef _HW_QUEUE_H
#define _HW_QUEUE_H

/*
 * Generic queue used to maintain current information about compute kernel
 * executions.
 */
class HWQueue
{
public:
	HWQueue(HWQueueConfig* p_config) : config(p_config) {}
	virtual ~HWQueue()
	{
		clear();
		delete config;
	}

	/* Configuration information */
	void printConfiguration() const;
	virtual size_t platform() const { return config->platform; }
	virtual size_t device() const { return config->device; }
	virtual size_t computeUnits() const { return config->computeUnits; }
	virtual size_t maxRunning() const { return config->maxRunning; }
	virtual bool canPartition() const = 0;

	/* Return the number of running or queued jobs */
	size_t numRunning() const { return runningJobs.size(); }
	size_t numQueued() const { return waitingJobs.size(); }

	/* Return whether or not the queue wants to run the specified job */
	bool canRun(Job* job) const;

	/* Add jobs to the running list or waiting queue */
	void running(Job* job);
	void enqueue(Job* job);

	Job* queued(size_t num) { return waitingJobs[num]; }
	Job* operator[](size_t num) { return queued(num); }

	/* Return whether or not the job for the specified PID is running */
	bool isRunning(pid_t clientPID) const;

	/* Remove entries from the running list */
	Job* finished(pid_t clientPID);

	/* Remove entries from the waiting queue */
	Job* dequeue();
	Job* remove(size_t num);

	/* Clear all entries from the ready list & waiting queue */
	void clear();

protected:
	HWQueueConfig* config;
	std::vector<Job*> runningJobs;
	std::vector<Job*> waitingJobs;
};

/* Class that models interference for CPUs. */
class CPUQueue : public HWQueue
{
public:
	CPUQueue(HWQueueConfig* p_config);
	virtual bool canPartition() const { return config->dynamicPartitioning; }
};

/* Class that models interference for GPUs. */
class GPUQueue : public HWQueue
{
public:
	GPUQueue(HWQueueConfig* p_config);
	virtual bool canPartition() const { return false; }
};

#endif /* _HW_QUEUE_H */

