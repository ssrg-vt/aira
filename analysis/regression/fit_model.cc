#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <gsl/gsl_matrix.h>

#include "data.hh"
#include "model.hh"
#include "transform.hh"

///////////////////////////////////////////////////////////////////////////////
// Global configuration
///////////////////////////////////////////////////////////////////////////////

#define NO_SCALE 0
#define RANGE_SCALE 1
#define STDDEV_SCALE 2

static size_t numArches = 3;
static std::vector<std::string> dataFiles;
static std::vector<Datafile*> data;
static bool save = false;
static std::string modelDir = "./models/";
static std::string transformFile = "transform.json";
static bool zeroMean = false;
static int scaleRange = NO_SCALE;
static bool ratio = false;

///////////////////////////////////////////////////////////////////////////////
// Help & argument parsing
///////////////////////////////////////////////////////////////////////////////

void printHelp()
{
	std::cout << "fit_model - fit regression models based on experimental data\n"
		"Usage: ./fit_model [ OPTIONS ]\n\n"

		"Options:\n"
		"  -h/--help       : print help & exit\n"
		"  --arch num      : number of architectures\n"
		"  --data CSV-list : a list of filenames to use for fitting/testing the model\n\n"

		"  --save          : save transform & fitting data\n"
		"  --dir directory : directory in which to save models (used with \"--save\")\n\n"

		"  --zero-mean     : make features have zero mean\n"
		"  --scale type    : scale ranges (either 'range' or 'stddev')\n"
		"  --ratio         : convert labels to ratios (first label : other label)\n\n";
}

void parseArgs(int argc, char** argv)
{
	bool skip = false;
	for(int i = 1; i < argc; i++)
	{
		if(skip) {
			skip = false;
		} else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printHelp();
			exit(0);
		} else if(!strcmp(argv[i], "--arch")) {
			numArches = atoi(argv[i]);
		} else if(!strcmp(argv[i], "--data")) {
			std::stringstream ss(argv[i+1]);
			std::string file;
			while(std::getline(ss, file, ',')) {
				dataFiles.push_back(file);
				if(ss.peek() == ',')
					ss.ignore();
			}
			skip = true;
		} else if(!strcmp(argv[i], "--save")) {
			save = true;
		} else if(!strcmp(argv[i], "--dir")) {
			modelDir = argv[i+1];
			skip = true;
		} else if(!strcmp(argv[i], "--zero-mean")) {
			zeroMean = true;
		} else if(!strcmp(argv[i], "--scale")) {
			if(!strcmp(argv[i+1], "range"))
				scaleRange = RANGE_SCALE;
			else if(!strcmp(argv[i+1], "stddev"))
				scaleRange = STDDEV_SCALE;
			else
				std::cerr << "Unknown scale type '" << argv[i+1] << "'" << std::endl;
			skip = true;
		} else if(!strcmp(argv[i], "--ratio")) {
			ratio = true;
		} else {
			std::cout << "Unknown parameter '" << argv[i] << "'" << std::endl;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

void loadData()
{
	Datafile* df = NULL;
	for(std::string file : dataFiles)
	{
		df = new Datafile(numArches);
		df->load(file);
		data.push_back(df);
	}
}

gsl_matrix* accumAllObs()
{
	size_t totalRows = 0;
	for(Datafile* df : data)
		totalRows += df->numRows();
	gsl_matrix* allObs = gsl_matrix_alloc(totalRows, data[0]->numFeatures());

	size_t allObsRow = 0;
	for(Datafile* df : data)
	{
		const gsl_matrix* dfFeatures = df->getFeatures();
		for(size_t dfRow = 0; dfRow < df->numRows(); dfRow++, allObsRow++)
			for(size_t col = 0; col < allObs->size2; col++)
				gsl_matrix_set(allObs, allObsRow, col, gsl_matrix_get(dfFeatures, dfRow, col));
	}

	return allObs;
}

Transform* zeroMeans(gsl_matrix* allObs)
{
	Transform* trans = new ZeroMean(allObs->size2);
	trans->calculate(allObs);
	return trans;
}

Transform* scaleRanges(gsl_matrix* allObs)
{
	Transform* trans;
	switch(scaleRange)
	{
	case RANGE_SCALE:
		trans = new RangeScale(allObs->size2);
		break;
	case STDDEV_SCALE:
		trans = new StdDevScale(allObs->size2);
		break;
	default:
		return NULL;
	}
	trans->calculate(allObs);
	return trans;
}

std::string toModelName(const std::string& filename)
{
	std::string copy(filename);
	copy = copy.replace(copy.begin(), copy.end()-8, "./");
	copy = copy.replace(copy.end()-3, copy.end(), "json");
	return modelDir + copy;
}

void leaveOneOutFitting()
{
	// TODO sanity check fitting?
	std::cout << "############################" << std::endl
						<< "# FITTING & ANALYZING MODELS" << std::endl
						<< "############################" << std::endl;

	for(Datafile* df : data)
	{
		std::vector<Datafile*> trainData;
		for(Datafile* odf : data)
			if(odf != df)
				trainData.push_back(odf);

		LinearRegressor lr(numArches);
		lr.fitModel(trainData);
		lr.evaluate(df);
		lr.report();
		if(save)
		{
			std::string modelFile = toModelName(df->getFilename());
			lr.save(modelFile);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Driver
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	parseArgs(argc, argv);
	loadData();

	// Create directory for saving
	if(save)
	{
		struct stat st;
		if(stat(modelDir.c_str(), &st) == -1)
			mkdir(modelDir.c_str(), 0700);
	}

	// Generate & apply transforms to features
	if(zeroMean || scaleRange)
	{
		TransformManager tm;
		gsl_matrix* allObs = accumAllObs();
		if(zeroMean) tm.addTransform(zeroMeans(allObs));
		if(scaleRange) tm.addTransform(scaleRanges(allObs));
		for(Datafile* df : data) tm.applyTransforms(df->getFeatures());
		gsl_matrix_free(allObs);
		if(save)
		{
			std::string fname = modelDir + transformFile;
			tm.save(fname);
		}
	}

	// Apply transforms to labels
	if(ratio)
		for(Datafile* df : data)
			df->labelsToRatios();

	// Fit models using leave-one-out cross-validation
	leaveOneOutFitting();
	return 0;
}

