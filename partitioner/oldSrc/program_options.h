/*
 * Class to handle input arguments using the Boost Program Options library.
 */

#ifndef _PROGRAM_OPTIONS_H
#define _PROGRAM_OPTIONS_H

#include "boost/program_options.hpp"

class ProgramOptions {

private:
	boost::program_options::options_description desc;
	boost::program_options::variables_map vm;

public:
	ProgramOptions(int argc, char** argv);

	bool printHelp();
	bool runASTConsistencyTests();
	bool printCallGraph();
	bool printAST();
	bool printFullAST();
	std::string getMPIHeader();
};

#endif
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
