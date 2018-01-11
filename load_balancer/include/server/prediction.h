/*****************************************************************************/
/* Machine Learning Interface                                                */
/*                                                                           */
/* This file describes the interface to utilize various machine learning     */
/* models trained offline.                                                   */
/*****************************************************************************/

#ifndef _MACHINE_LEARNING
#define _MACHINE_LEARNING

#include <opencv/cv.h>
#include <opencv/ml.h>

/* Popcorn ML */
#include "transform.hh"
#include "predict.hh"

#include "server/system.h"

/* Available predictors */
#define PREDICTORS \
	X(NN = 0, "artificial neural net") \
	X(ALWAYS_CPU, "always predict CPU as best") \
	X(ALWAYS_GPU, "always predict GPU as best") \
	X(EXACT_RT, "hard-coded runtime") \
	X(EXACT_ENERGY, "hard-coded energy consumption") \
	X(EXACT_EDP, "hard-coded energy-delay product") \

enum predictor {
#define X(a, b) a,
	PREDICTORS
#undef X
	NUM_PREDICTORS
};

extern const char* predictorNames[];

///////////////////////////////////////////////////////////////////////////////
// Base class for machine learning models.
///////////////////////////////////////////////////////////////////////////////

class Predictor {
public:
	Predictor(int p_numDevices) : numDevices(p_numDevices) {}
	virtual ~Predictor() {}
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions) = 0;

protected:
	int numDevices;
};

///////////////////////////////////////////////////////////////////////////////
// Always predicts CPU will be the fastest by large margin
// (prevents possibility of switching architectures)
///////////////////////////////////////////////////////////////////////////////

class AlwaysCPU : public Predictor {
public:
	AlwaysCPU(int p_numDevices = prediction_slots)
		: Predictor(p_numDevices) {}
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions);
};

///////////////////////////////////////////////////////////////////////////////
// Always predicts GPU will be the fastest by large margin
// (prevents possibility of switching architectures)
///////////////////////////////////////////////////////////////////////////////

class AlwaysGPU : public Predictor {
public:
	AlwaysGPU(int p_numDevices = prediction_slots)
		: Predictor(p_numDevices) {}
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions);
};

///////////////////////////////////////////////////////////////////////////////
// Encodes exact runtimes for each benchmark (use for testing policy
// effectiveness)
///////////////////////////////////////////////////////////////////////////////

class ExactRuntime : public Predictor {
public:
	ExactRuntime(int p_numDevices = prediction_slots)
		: Predictor(p_numDevices) {}
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions);
};

///////////////////////////////////////////////////////////////////////////////
// Encodes exact energy consumption for each benchmark (use for testing policy
// effectiveness)
///////////////////////////////////////////////////////////////////////////////

class ExactEnergy : public Predictor {
public:
	ExactEnergy(int p_numDevices = prediction_slots)
		: Predictor(p_numDevices) {}
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions);
};

///////////////////////////////////////////////////////////////////////////////
// Encodes exact energy-delay product for each benchmark (use for testing
// policy effectiveness)
///////////////////////////////////////////////////////////////////////////////

class ExactEDP : public Predictor {
public:
	ExactEDP(int p_numDevices = prediction_slots)
		: Predictor(p_numDevices) {}
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions);
};

///////////////////////////////////////////////////////////////////////////////
// OpenMP (version 1) functionality
///////////////////////////////////////////////////////////////////////////////

class NeuralNetPredictor : public Predictor {
public:
	NeuralNetPredictor(std::string& modelFN,
										 std::string& transFN,
										 int numDevices = prediction_slots);
	virtual void predict(struct kernel_features& feats,
											 std::vector<float>& predictions);

private:
	int numInputs;
	int numLayers;
	TransformManager trans;
	NeuralNetwork model;
};

#endif /* _MACHINE_LEARNING */

