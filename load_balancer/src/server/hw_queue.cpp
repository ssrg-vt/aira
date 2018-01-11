#include <iostream>
#include "server/server.h"

///////////////////////////////////////////////////////////////////////////////
// HWQueue implementation
///////////////////////////////////////////////////////////////////////////////

void HWQueue::printConfiguration() const
{
	std::cout << "  "
		<< config->platform << "/" << config->device << ": "
		<< config->computeUnits << " compute unit(s), "
		<< config->maxRunning << " can run concurrently, dynamic partitioning is "
		<< (config->dynamicPartitioning ? "enabled" : "disabled")  << std::endl;
}

bool HWQueue::canRun(Job* job) const
{	
	// For now, check based on the max number allowed to run
	if(runningJobs.size() < config->maxRunning)
		return true;
	else
		return false;
}

void HWQueue::running(Job* job)
{
	clock_gettime(CLOCK_MONOTONIC, &job->start);
	job->alloc.platform = this->config->platform;
	job->alloc.device = this->config->device;
	job->alloc.compute_units = this->config->computeUnits;
	runningJobs.push_back(job);
}

void HWQueue::enqueue(Job* job)
{
	clock_gettime(CLOCK_MONOTONIC, &job->queued);
	waitingJobs.push_back(job);
}

bool HWQueue::isRunning(pid_t clientPID) const
{
	for(Job* job : runningJobs)
		if(job->client == clientPID)
			return true;
	return false;
}

Job* HWQueue::finished(pid_t clientPID)
{
	Job* job;
	std::vector<Job*>::iterator it;
	for(it = runningJobs.begin(); it != runningJobs.end(); it++)
		if((*it)->client == clientPID)
			break;
	if(it == runningJobs.end()) return NULL;
	job = *it;

	clock_gettime(CLOCK_MONOTONIC, &job->end);
	runningJobs.erase(it);
	return job;
}

Job* HWQueue::dequeue()
{
	Job* job = waitingJobs.front();
	waitingJobs.erase(waitingJobs.begin());
	return job;
}

Job* HWQueue::remove(size_t num)
{
	if(num <= 0 || waitingJobs.size() <= num) return NULL;

	Job* job = waitingJobs[num];
	std::vector<Job*>::iterator it;
	for(it = waitingJobs.begin(); num > 0; num--, it++);
	waitingJobs.erase(it);
	return job;
}

void HWQueue::clear()
{
	while(!waitingJobs.empty())
	{
		delete waitingJobs.back();
		waitingJobs.pop_back();
	}

	while(!runningJobs.empty())
	{
		delete runningJobs.back();
		runningJobs.pop_back();
	}
}

///////////////////////////////////////////////////////////////////////////////
// CPUQueue implementation
///////////////////////////////////////////////////////////////////////////////

/* Default constructor */
CPUQueue::CPUQueue(HWQueueConfig* p_config) : HWQueue(p_config) {}

///////////////////////////////////////////////////////////////////////////////
// GPUQueue implementation
///////////////////////////////////////////////////////////////////////////////

/* Default constructor */
GPUQueue::GPUQueue(HWQueueConfig* p_config) : HWQueue(p_config) {}

