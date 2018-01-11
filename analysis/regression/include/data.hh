#include <vector>
#include <fstream>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_fit.h>

#include "persistent.hh"

#ifndef _DATAFILE_H
#define _DATAFILE_H

class Datafile : public Persistent
{
public:
	Datafile(size_t numArches);
	~Datafile();

	size_t numRows() const { return rows; }
	size_t numFeatures() const { return features; }
	size_t numLabels() const { return labels; }
	gsl_matrix* getFeatures() { return x; }
	gsl_vector* getLabels(int arch) { return y[arch]; }
	std::vector<gsl_vector*>& getLabels() { return y; }
	const std::string& getFilename() const { return filename; }

	void labelsToRatios();

	virtual int save(std::string& filename);
	virtual int load(std::string& filename);

private:
	std::string filename;

	/* Data size & shape */
	size_t rows;
	size_t cols;
	size_t features;
	size_t labels;

	/* Data (named according to GSL library) */
	gsl_matrix* x; // Features, i.e. independent predictors
	std::vector<gsl_vector*> y; // Labels, i.e. observed values

	/* Private API */
	size_t numLines(std::ifstream& file);
	size_t numCols(std::ifstream& file);
	void loadData(std::ifstream& file);
};

#endif /* _DATAFILE_H */
