#include <iostream>
#include <limits>
#include <cassert>
#include <ctime>

#include "model.hh"

///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

#define CLOCK CLOCK_MONOTONIC
#define toNS( ts ) ((ts.tv_sec * 1000000000) + ts.tv_nsec)

///////////////////////////////////////////////////////////////////////////////
// RegressionModel
///////////////////////////////////////////////////////////////////////////////

RegressionModel::RegressionModel(size_t numArches) : arches(numArches)
{
	confusionMatrix = new int[arches * arches];
	for(size_t i = 0; i < arches * arches; i++)
		confusionMatrix[i] = 0;
}

RegressionModel::~RegressionModel()
{
	delete confusionMatrix;
}

///////////////////////////////////////////////////////////////////////////////
// LinearRegressor public API
///////////////////////////////////////////////////////////////////////////////

LinearRegressor::LinearRegressor(size_t numArches)
	: RegressionModel(numArches), chisq(numArches), fittingTime(0),
		predictionTime(0), numPredictions(0), correctPredictions(0),
		avgError(numArches), features(NULL)
{
	predictions = gsl_vector_alloc(arches);
	errors = gsl_vector_alloc(arches);
}

LinearRegressor::~LinearRegressor()
{
	for(size_t i = 0; i < arches; i++)
	{
		gsl_vector_free(c[i]);
		gsl_matrix_free(cov[i]);
	}
	gsl_vector_free(features);
	gsl_vector_free(predictions);
	gsl_vector_free(errors);
}

int LinearRegressor::fitModel(std::vector<Datafile*>& fitting)
{
	// Allocate storage for observations
	size_t totalRows = 0;
	size_t numFeatures = fitting[0]->numFeatures();
	for(Datafile* df : fitting)
	{
		assert(df->numFeatures() == numFeatures);
		totalRows += df->numRows();
	}
	gsl_matrix* allObs = gsl_matrix_alloc(totalRows, fitting[0]->numFeatures());
	features = gsl_vector_alloc(numFeatures);

	// Allocate storage for model data
	std::vector<gsl_vector*> labels;
	std::vector<gsl_multifit_linear_workspace*> ws;
	for(size_t i = 0; i < arches; i++)
	{
		labels.push_back(gsl_vector_alloc(totalRows));
		c.push_back(gsl_vector_alloc(numFeatures));
		cov.push_back(gsl_matrix_alloc(numFeatures, numFeatures));
		ws.push_back(gsl_multifit_linear_alloc(totalRows, numFeatures));
	}

	// Populate w/ features & labels for fitting
	size_t allObsRow = 0;
	for(Datafile* df : fitting)
	{
		const gsl_matrix* dfFeatures = df->getFeatures();
		const std::vector<gsl_vector*>& dfLabels = df->getLabels();
		for(size_t dfRow = 0; dfRow < df->numRows(); dfRow++, allObsRow++)
		{
			// Features
			for(size_t col = 0; col < numFeatures; col++)
				gsl_matrix_set(allObs, allObsRow, col,
											 gsl_matrix_get(dfFeatures, dfRow, col));
			// Labels
			for(size_t i = 0; i < arches; i++)
				gsl_vector_set(labels[i], allObsRow,
											 gsl_vector_get(dfLabels[i], dfRow));
		}
	}

	// Fit models
	struct timespec start, end;
	clock_gettime(CLOCK, &start);
	for(size_t i = 0; i < arches; i++)
		assert(!gsl_multifit_linear(allObs, labels[i], c[i], cov[i], &chisq[i], ws[i]));
	clock_gettime(CLOCK, &end);
	fittingTime = toNS(end) - toNS(start);

	for(size_t i = 0; i < arches; i++)
	{
		gsl_vector_free(labels[i]);
		gsl_multifit_linear_free(ws[i]);
	}
	gsl_matrix_free(allObs);
	return 0; // No problem mahn
}

/*
 * Evaluate the model using the specified features.  This function is exposed
 * as an interface for applications that do not use GSL.
 */
std::vector<double> LinearRegressor::evaluate(std::vector<double>& features)
{
	std::vector<double> predictions(arches);
	assert(features.size() == this->features->size);

	for(size_t i = 0; i < features.size(); i++)
		gsl_vector_set(this->features, i, features[i]);
	evaluate();
	for(size_t i = 0; i < arches; i++)
		predictions[i] = gsl_vector_get(this->predictions, i);

	return predictions; // No problem mahn
}

int LinearRegressor::evaluate(Datafile* testing)
{
	struct timespec start, end;
	const gsl_matrix* obs = testing->getFeatures();
	const std::vector<gsl_vector*>& labels = testing->getLabels();
	int bestArch = 0, bestPredicted = 0;
	double bestVal;
	for(size_t j = 0; j < avgError.size(); j++)
		avgError[j] = 0.0;

	clock_gettime(CLOCK, &start);
	for(size_t i = 0; i < testing->numRows(); i++)
	{
		// Find actual best architecture
		bestVal = std::numeric_limits<double>::max();
		for(size_t j = 0; j < labels.size(); j++) {
			if(gsl_vector_get(labels[j], i) < bestVal) {
				bestArch = j;
				bestVal = gsl_vector_get(labels[j], i);
			}	
		}

		// Evaluate model & find predicted best architecture
		bestVal = std::numeric_limits<double>::max();
		for(size_t j = 0; j < testing->numFeatures(); j++)
			gsl_vector_set(this->features, j, gsl_matrix_get(obs, i, j));
		evaluate();
		for(size_t j = 0; j < labels.size(); j++) {
			if(gsl_vector_get(predictions, j) < bestVal) {
				bestPredicted = j;
				bestVal = gsl_vector_get(predictions, j);
			}
		}

		for(size_t j = 0; j < avgError.size(); j++)
			avgError[j] += gsl_vector_get(errors, j);
		if(bestPredicted == bestArch)
			correctPredictions++;
		confusionMatrix[(bestArch * arches) + bestPredicted]++;
		numPredictions++;
	}
	clock_gettime(CLOCK, &end);
	predictionTime = (toNS(end) - toNS(start)) / testing->numRows();

	for(size_t j = 0; j < avgError.size(); j++)
		avgError[j] /= numPredictions;

	return 0; // No problem mahn
}

void LinearRegressor::report()
{
	std::cout << "Prediction report:" << std::endl <<
		"Time for fitting parameters: " << fittingTime << "ns" << std::endl <<
		"Time for making predictions: " << predictionTime << "ns" << std::endl <<
		"Prediction accuracy: " << correctPredictions << "/" << numPredictions << std::endl <<
		"Average Error:";
	for(size_t i = 0; i < arches; i++)
		std::cout << " " << avgError[i];
	std::cout << std::endl << "Confusion matrix:" << std::endl;
	for(size_t i = 0; i < arches; i++) {
		for(size_t j = 0; j < arches; j++)
			std::cout << confusionMatrix[(i * arches) + j] << " ";
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

int LinearRegressor::save(std::string& filename)
{
	using boost::property_tree::ptree;
	using boost::property_tree::write_json;

	ptree pt;
	pt.put("type", "linear");
	pt.put("arches", arches);
	pt.put("parameters", c[0]->size);
	for(size_t i = 0; i < arches; i++)
	{
		std::stringstream ss;
		std::string num, data;

		ss << i;
		ss >> num;
		ss.clear();
		ss << gsl_vector_get(c[i], 0);
		ss >> data;
		for(size_t j = 1; j < c[i]->size; j++)
		{
			std::string val;
			ss.clear();
			ss << gsl_vector_get(c[i], j);
			ss >> val;
			data += "," + val;
		}
		pt.put("c." + num, data);

		data = "";
		for(size_t j = 0; j < cov[i]->size1; j++)
		{
			std::string val;
			ss.clear();
			ss << gsl_matrix_get(cov[i], j, 0);
			ss >> val;
			data += val;
			for(size_t k = 0; k < cov[i]->size2; k++)
			{
				ss.clear();
				ss << gsl_matrix_get(cov[i], j, k);
				ss >> val;
				data += "," + val;
			}
			data += "\n";
		}
		pt.put("cov." + num, data);
	}

	write_json(filename, pt);
	return 0; // No problem mahn
}

int LinearRegressor::load(std::string& filename)
{
	using boost::property_tree::ptree;
	using boost::property_tree::read_json;

	ptree pt;
	

	return 0; // No problem mahn
}

///////////////////////////////////////////////////////////////////////////////
// LinearRegressor private API
///////////////////////////////////////////////////////////////////////////////

/*
 * Evaluate the model using the specified features.
 *
 * Note: operates on private scratchpad memory, needs to be set prior to
 * calling!
 */
void LinearRegressor::evaluate()
{
	for(size_t i = 0; i < arches; i++)
		assert(!gsl_multifit_linear_est(features, c[i], cov[i],
																		&predictions->data[i], &errors->data[i]));
}

