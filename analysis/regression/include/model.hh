#include <vector>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>

#include "data.hh"

#ifndef _MODEL_H
#define _MODEL_H

class RegressionModel : public Persistent
{
public:
	RegressionModel(size_t numArches);
	virtual ~RegressionModel();

	virtual int fitModel(std::vector<Datafile*>& fitting) = 0;
	virtual std::vector<double> evaluate(std::vector<double>& features) = 0;
	virtual int evaluate(Datafile* testing) = 0;
	virtual void report() = 0;

	virtual int save(std::string& filename) = 0;
	virtual int load(std::string& filename) = 0;

	const int* getConfusionMatrix() const { return confusionMatrix; }

protected:
	size_t arches;
	int* confusionMatrix;
};

class LinearRegressor : public RegressionModel
{
public:
	LinearRegressor(size_t numArches);
	virtual ~LinearRegressor();

	virtual int fitModel(std::vector<Datafile*>& fitting);
	virtual std::vector<double> evaluate(std::vector<double>& features);
	virtual int evaluate(Datafile* testing);
	virtual void report();

	virtual int save(std::string& filename);
	virtual int load(std::string& filename);

private:
	/* Per-architecture model data (named according to GSL library) */
	std::vector<gsl_vector*> c; // Best-fit parameters
	std::vector<gsl_matrix*> cov; // Covariance matrix
	std::vector<double> chisq; // X^2 (sum of squares of residuals) from fitting

	/* Statistics */
	unsigned long long fittingTime;
	unsigned long long predictionTime;
	size_t numPredictions;
	size_t correctPredictions;
	std::vector<double> avgError;

	/* Scratchpad memory to prevent continual alloc/de-alloc */
	gsl_vector* features;
	gsl_vector* predictions;
	gsl_vector* errors;

	void evaluate();
};

#endif /* _MODEL_H */
