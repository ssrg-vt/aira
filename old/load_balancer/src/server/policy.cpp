#include <cassert>
#include <cmath>
#include <iostream>

#include "server/server.h"
#include "server/util.h"

using namespace std;

/*
 * Define common worst-initalization & comparison operator in common place, so
 * that if prediction format changes (e.g. from runtime to unit-less value)
 * all policies can be changed at once.
 */
#define INIT_WORST 1e15
#define BETTER( val, prev_best ) (val < prev_best)

///////////////////////////////////////////////////////////////////////////////
// StaticPolicy implementation
///////////////////////////////////////////////////////////////////////////////

struct resource_alloc StaticPolicy::apply(float start_time,
																					vector<float>& predictions,
																					struct kernel_features* features,
																					float* predicted_end)
{
	assert(predicted_end);

	struct resource_alloc alloc;
	float best_prediction = INIT_WORST;
	int best_arch = 0;
	for(size_t i = 0; i < predictions.size(); i++)
	{
		if(BETTER(predictions[i], best_prediction))
		{
			best_prediction = predictions[i];
			best_arch = i;
		}
	}

	alloc = index_to_alloc(best_arch);
	alloc.compute_units = computeUnits[best_arch];
	*predicted_end = start_time + best_prediction;
#ifdef _SERVER_VERBOSE
	std::cout << ", predicted end: " << *predicted_end;
#endif

	return alloc;
}

///////////////////////////////////////////////////////////////////////////////
// PartitionPolicy implementation
///////////////////////////////////////////////////////////////////////////////

struct resource_alloc PartitionPolicy::apply(float start_time,
																			 vector<float>& predictions,
																			 struct kernel_features* features,
																			 float* predicted_end)
{
	assert(predicted_end);

	struct resource_alloc alloc;
	float best_prediction = INIT_WORST;
	int best_arch = 0;
	for(size_t i = 0; i < predictions.size(); i++)
	{
		// Note: since we're not adjusting the architecture, don't model decreased
		// runtime due to fewer threads when picking architecture
		if(BETTER(predictions[i], best_prediction))
		{
			best_prediction = predictions[i];
			best_arch = i;
		}
	}

	alloc = index_to_alloc(best_arch);
	float num_cores = computeUnits[best_arch];
	float num_allocs = queues[best_arch]->getSize() + 1;
	alloc.compute_units = ceil(num_cores / num_allocs);
	*predicted_end = start_time +
		best_prediction * num_cores / (float)alloc.compute_units;
#ifdef _SERVER_VERBOSE
	std::cout << ", predicted end: " << *predicted_end;
#endif

	return alloc;
}

///////////////////////////////////////////////////////////////////////////////
// ArchAdjPolicy implementation
///////////////////////////////////////////////////////////////////////////////

struct resource_alloc ArchAdjPolicy::apply(float start_time,
																		 vector<float>& predictions,
																		 struct kernel_features* features,
																		 float* predicted_end)
{
	assert(features);
	assert(predicted_end);

	struct resource_alloc alloc;
	float best_prediction = INIT_WORST;
	int best_arch = 0;
	for(size_t i = 0; i < predictions.size(); i++)
	{
		int inter_cores = computeUnits[i] + queues[i]->getSize();
		float inter_prediction = queues[i]->modelInterference(features,
																													start_time,
																													predictions[i],
																													inter_cores);

#ifdef _SERVER_VERBOSE
		std::cout << ", " << i << ": " << inter_prediction;
#endif

		if(BETTER(inter_prediction, best_prediction))
		{
			best_prediction = inter_prediction;
			best_arch = i;
		}
	}

	alloc = index_to_alloc(best_arch);
	alloc.compute_units = computeUnits[best_arch];
	*predicted_end = start_time + best_prediction;
#ifdef _SERVER_VERBOSE
	std::cout << ", predicted end: " << *predicted_end;
#endif

	return alloc;
}

///////////////////////////////////////////////////////////////////////////////
// PartitionAndAdjPolicy implementation
///////////////////////////////////////////////////////////////////////////////

struct resource_alloc PartitionAndAdjPolicy::apply(float start_time,
																						 vector<float>& predictions,
																						 struct kernel_features* features,
																						 float* predicted_end)
{
	assert(features);
	assert(predicted_end);

	struct resource_alloc alloc;
	float best_prediction = INIT_WORST, num_cores, num_execs;
	int best_arch = 0;
	for(size_t i = 0; i < predictions.size(); i++)
	{
		num_cores = computeUnits[i];
		num_execs = queues[i]->getSize() + 1;
		size_t alloc_cores = ceil(num_cores / num_execs);
		size_t inter_cores = alloc_cores + queues[i]->getSize();
		float inter_prediction = queues[i]->modelInterference(features,
																													start_time,
																													predictions[i],
																													inter_cores);

		// TODO this is hacked, assumes CPU == device 0
		if(i == 0 && alloc_cores < computeUnits[0]) // Adjust for fewer threads
			inter_prediction *= (float)computeUnits[0] / (float)alloc_cores;

#ifdef _SERVER_VERBOSE
		std::cout << ", " << i << ": " << inter_prediction;
#endif

		if(BETTER(inter_prediction, best_prediction))
		{
			best_prediction = inter_prediction;
			best_arch = i;
		}
	}

	alloc = index_to_alloc(best_arch);
	num_cores = computeUnits[best_arch];
	num_execs = queues[best_arch]->getSize() + 1;
	alloc.compute_units = ceil(num_cores / num_execs);
	*predicted_end = start_time + best_prediction;
#ifdef _SERVER_VERBOSE
	std::cout << ", predicted end: " << *predicted_end;
#endif

	return alloc;
}

const char* policyNames[] = {
#define X(a, b) b,
POLICIES
#undef X
};

