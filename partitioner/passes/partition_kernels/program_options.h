/*
 * program_options.h
 *
 *  Created on: May 8, 2013
 *      Author: rlyerly
 */

#ifndef PROGRAM_OPTIONS_H_
#define PROGRAM_OPTIONS_H_

#include "boost/program_options.hpp"

class ProgramOptions {
public:
	ProgramOptions(int, char**);
	string getMpiHeaderLocation();
	string getMpiOutputFile();
	string getMpiPartitionName();
	string getGpuHeaderLocation();
	string getGpuErrorHeaderLocation();
	string getGpuOutputFile();
	string getGpuPartitionName();

private:
	boost::program_options::options_description desc;
	boost::program_options::variables_map vm;
};

#endif /* PROGRAM_OPTIONS_H_ */
