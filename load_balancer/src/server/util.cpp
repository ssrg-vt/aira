/*
 * Implementation of utility functions.
 *
 * Author: Rob Lyerly
 * Date: 9/13/2015
 */

#include <string>
#include <algorithm>
#include <cstdlib>
#include <cstdio>

#include "retvals.h"

#include "server/server.h"
#include "server/util.h"
#include "server/system.h"

/*
 * Performance threshold used to determine if two architectures are within
 * an acceptable performance range (i.e. if computation can be switched
 * between two architectures.
 */
static float perf_threshold = 0.2f;

/*
 * Set the acceptable performance difference between two architectures.  This
 * determines how willing the load-balancer is to switch execution between
 * heterogeneous architectures.
 */
void utility::set_threshold(float new_threshold)
{
	perf_threshold = new_threshold;
}

/*
 * Returns true if prediction b is within the performance threshold of
 * prediction a.
 */
bool utility::within_threshold(float a, float b)
{
	if(b >= (a - (a * perf_threshold))) return true;
	else return false;
}

/*
 * Comparison function used to sort candidate architecture based on the job's
 * predicted performance on each.
 */
bool utility::better_candidate(const std::pair<int, float>& a,
															 const std::pair<int, float>& b)
{
	if(b.second > a.second) return true;
	else return false;
}

/*
 * Return true if a given prediction slot has an available HW queue, or false
 * otherwise.
 */
bool utility::is_available(size_t pred_slot)
{
	return utility::alloc_to_index(system_devices[pred_slot]).size() != 0;
}

/*
 * Return true if two hardware queues are equivalent logical devices (i.e. a
 * 12C CPU split into two 6C CPU devices), or false otherwise.
 */
bool utility::same_device(size_t q1, size_t q2)
{
	if(queues[q1]->platform() == queues[q2]->platform() &&
		 queues[q1]->device() == queues[q2]->device() &&
		 queues[q1]->computeUnits() == queues[q2]->computeUnits())
		return true;
	return false;
}

/*
 * Return the length of each architecture's allocation queue in human-readable
 * form.
 */
std::string utility::queue_sizes()
{
	if(queues.size() <= 0) CHECK_ERR(FAILURE);
	std::string sizes = std::to_string(queues[0]->numRunning()) +	"/" +
											std::to_string(queues[0]->numQueued());
	for(size_t i = 1; i < queues.size(); i++)
		sizes += " " + std::to_string(queues[i]->numRunning()) + "/" +
									 std::to_string(queues[i]->numQueued());
	return sizes;
}

/*
 * Convert a platform/device/CU configuration into a queue index.  Because we
 * can have multiple copies of the same device (e.g. a 12C CPU broken down into
 * two identical 6C sub-devices), we return a vector of devices.
 */
std::vector<size_t> utility::alloc_to_index(struct resource_alloc alloc)
{
	std::vector<size_t> indexes;
	for(size_t i = 0; i < queues.size(); i++)
		if(alloc.platform == queues[i]->platform() &&
			 alloc.device == queues[i]->device() &&
			 alloc.compute_units == queues[i]->computeUnits())
			indexes.push_back(i);	
	return indexes;
}

/*
 * Convert a raw index into a platform + device number.
 */
struct resource_alloc utility::index_to_alloc(size_t index)
{
	struct resource_alloc alloc = {
		.platform = 0,
		.device = 0,
		.compute_units = 0
	};
	if(index < 0 || queues.size() <= index) CHECK_ERR(ALLOC_ERR);
	alloc.platform = queues[index]->platform();
	alloc.device = queues[index]->device();
	alloc.compute_units = queues[index]->computeUnits();
	return alloc;
}

/*
 * For job 'j' in queue 'q', get prediction for other queue 'q_to_predict'
 */
float utility::get_prediction(size_t q, size_t j, size_t q_to_predict)
{
	// Find prediction slot
	for(size_t i = 0; i < prediction_slots; i++) {
		if(queues[q_to_predict]->platform() == system_devices[i].platform &&
			 queues[q_to_predict]->device() == system_devices[i].device &&
			 queues[q_to_predict]->computeUnits() == system_devices[i].compute_units) {
			return queues[q]->queued(j)->predictions[i];
		}
	}
	CHECK_ERR(FAILURE); // Couldn't find the prediction
	return -1.0;
}

/*
 * Find candidate HW queues, sorted from best to worst.
 */
std::vector<size_t> utility::get_candidates(Job* job)
{
	// Get the best (i.e. preferred) architecture
	std::pair<size_t, float> best(0, -1000.0f);
	for(size_t i = 0; i < job->predictions.size(); i++) {
		std::pair<size_t, float> cur(i, job->predictions[i]);
		if(is_available(i) && utility::better_candidate(best, cur))
			best = cur;
	}

	// Find candidate prediction slots
	std::vector<std::pair<size_t, float> > candidates;
	candidates.push_back(best);
	for(size_t i = 0; i < job->predictions.size(); i++)
		if(i != best.first && is_available(i) &&
			 utility::within_threshold(best.second, job->predictions[i]))
			candidates.push_back(std::pair<size_t, float>(i, job->predictions[i]));
	std::sort(candidates.begin(), candidates.end(), utility::better_candidate);

	// Convert prediction slots into HW queues & return candidates
	std::vector<size_t> queues;
	for(std::pair<size_t, float> pair : candidates)
	{
		struct resource_alloc tmp = system_devices[pair.first];
		std::vector<size_t> tmp_queues = utility::alloc_to_index(tmp);
		for(size_t i : tmp_queues)
			queues.push_back(i);
	}
	return queues;
}

