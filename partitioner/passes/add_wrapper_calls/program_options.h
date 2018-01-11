/*
 * program_options.h
 *
 *  Created on: May 17, 2013
 *      Author: rlyerly
 */

#ifndef PROGRAM_OPTIONS_H_
#define PROGRAM_OPTIONS_H_

#include "boost/program_options.hpp"

namespace po = boost::program_options;

//Definitions of command line arguments
#define MM_WRAPPER_HEADER "mmWrapperHeader"

//Defaults for command-line args
#define DEF_MM_WRAPPER_HEADER "mm_wrapper.h"

class ProgramOptions {
public:
	ProgramOptions(int argc, char** argv);
	string getMMWrapperHeaderLocation();

private:
	po::options_description desc;
	po::variables_map vm;
};

#endif /* PROGRAM_OPTIONS_H_ */
