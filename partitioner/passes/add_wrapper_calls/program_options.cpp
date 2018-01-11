/*
 * program_options.cpp
 *
 *  Created on: May 17, 2013
 *      Author: rlyerly
 */

using namespace std;

#include "program_options.h"

ProgramOptions::ProgramOptions(int argc, char** argv)
{
	desc.add_options()
			(MM_WRAPPER_HEADER, po::value<string>(), "set memory-management wrapper header location")
	;

	po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	po::notify(vm);
}

string ProgramOptions::getMMWrapperHeaderLocation()
{
	if(vm.count(MM_WRAPPER_HEADER))
		return vm[MM_WRAPPER_HEADER].as<string>();
	else
		return DEF_MM_WRAPPER_HEADER;
}
