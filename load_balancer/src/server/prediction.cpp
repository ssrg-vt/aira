#include <iostream>

/* Heterogeneous load-balancer */
#include "server/server.h"
#include "server/util.h"

/* AIRA ML */
#include "mat.hh"

///////////////////////////////////////////////////////////////////////////////
// AlwaysCPU implementation
///////////////////////////////////////////////////////////////////////////////

/* Set CPU predictions so they appear significantly faster than GPU */
void AlwaysCPU::predict(struct kernel_features& feats,
												std::vector<float>& predictions)
{
	for(size_t i = 0; i < predictions.size(); i++)
	{
		if(device_types[i] == CL_DEVICE_TYPE_CPU)
			predictions[i] = 5.0f;
		else
			predictions[i] = 1.0f;
	}
}

///////////////////////////////////////////////////////////////////////////////
// AlwaysGPU implementation
///////////////////////////////////////////////////////////////////////////////

/* Set GPU predictions so they appear significantly faster than CPU */
void AlwaysGPU::predict(struct kernel_features& feats,
												std::vector<float>& predictions)
{
	for(size_t i = 0; i < predictions.size(); i++)
	{
		if(device_types[i] == CL_DEVICE_TYPE_GPU)
			predictions[i] = 5.0f;
		else
			predictions[i] = 1.0f;
	}	
}

///////////////////////////////////////////////////////////////////////////////
// ExactRuntime implementation
///////////////////////////////////////////////////////////////////////////////

/* Return exact runtime based on static training data */
void ExactRuntime::predict(struct kernel_features& feats,
													 std::vector<float>& predictions)
{
	for(size_t i = 0; i < predictions.size(); i++) {
		if(i != default_cpu)
			predictions[i] = runtime[default_cpu][feats.kernel] /
											 runtime[i][feats.kernel];
		else
			predictions[i] = 1.0f;
	}

#ifdef _SERVER_VERBOSE
	std::cout << "exact runtime (vs. CPU): " << std::fixed << predictions[0];
	for(size_t i = 1; i < predictions.size(); i++)
		std::cout << " " << predictions[i];
#endif
}

///////////////////////////////////////////////////////////////////////////////
// ExactEnergy implementation
///////////////////////////////////////////////////////////////////////////////

/* Return exact energy consumption based on static training data */
void ExactEnergy::predict(struct kernel_features& feats,
													std::vector<float>& predictions)
{
	for(size_t i = 0; i < predictions.size(); i++) {
		if(i != default_cpu)
			predictions[i] = energy[default_cpu][feats.kernel] /
											 energy[i][feats.kernel];
		else
			predictions[i] = 1.0f;
	}

#ifdef _SERVER_VERBOSE
	std::cout << "exact energy (vs. CPU): " << std::fixed << predictions[0];
	for(size_t i = 1; i < predictions.size(); i++)
		std::cout << " " << predictions[i];
#endif
}

///////////////////////////////////////////////////////////////////////////////
// ExactEDP implementation
///////////////////////////////////////////////////////////////////////////////

/* Return exact energy consumption based on static training data */
void ExactEDP::predict(struct kernel_features& feats,
											 std::vector<float>& predictions)
{
	float defaultEDP = runtime[default_cpu][feats.kernel] *
										 energy[default_cpu][feats.kernel];
	for(size_t i = 0; i < predictions.size(); i++) {
		if(i != default_cpu) {
			float curEDP = runtime[i][feats.kernel] * energy[i][feats.kernel];
			predictions[i] = defaultEDP / curEDP;
		}
		else
			predictions[i] = 1.0f;
	}

#ifdef _SERVER_VERBOSE
	std::cout << "exact energy-delay product (vs. CPU): " << std::fixed
						<< predictions[0];
	for(size_t i = 1; i < predictions.size(); i++)
		std::cout << " " << predictions[i];
#endif
}

///////////////////////////////////////////////////////////////////////////////
// NeuralNetPredictor implementation
///////////////////////////////////////////////////////////////////////////////

/* Constructor */
NeuralNetPredictor::NeuralNetPredictor(std::string& modelFN,
																			 std::string& transFN,
																			 int numDevices)
	: Predictor(numDevices), numInputs(NUM_FEATURES)
{
	model.load(modelFN);
	trans.load(transFN);
}

/* Make performance prediction using ML model */
void NeuralNetPredictor::predict(struct kernel_features& feats,
																 std::vector<float>& predictions)
{
	// 1. Populate input vector
	Row32F inputs(numInputs);
	for(size_t i = 0; i < NUM_FEATURES; i++)
		inputs.at(0) = feats.feature[i];

	// 2. Apply transform to features
	inputs = trans.apply(inputs);

	// 3. Evaluate model
	Row32F outputs = model.predict(inputs);

	// 4. Copy predictions to buffer
	for(int i = 0; i < numDevices; i++)
		predictions[i] = ((outputs.at(i) * (rt_max - rt_min)) + rt_min) + rt_mean;

#ifdef _SERVER_VERBOSE
	std::cout << "NN predictions:" << std::fixed << predictions[0];
	for(size_t i = 1; i < predictions.size(); i++)
		std::cout << " " << predictions[i];
#endif
}

const char* predictorNames[] = {
#define X(a, b) b,
PREDICTORS
#undef X
};

