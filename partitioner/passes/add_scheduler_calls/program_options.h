/*
 * program_options.h
 *
 *  Created on: Jun 7, 2013
 *      Author: rlyerly
 */

#ifndef PROGRAM_OPTIONS_H_
#define PROGRAM_OPTIONS_H_

#include "boost/program_options.hpp"

namespace po = boost::program_options;

/* Command-line options */
#define SCHED_HEADER "schedHeader"
#define FEATURES_FOLDER "featuresFolder"

class ProgramOptions {
public:
	ProgramOptions(int argc, char** argv);
	string getSchedulerHeaderLocation();
	bool getFeaturesFolder(string& buf);

private:
	po::options_description desc;
	po::variables_map vm;
};

#endif /* PROGRAM_OPTIONS_H_ */
