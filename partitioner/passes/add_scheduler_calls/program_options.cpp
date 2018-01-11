/*
 * program_options.cpp
 *
 *  Created on: Jun 7, 2013
 *      Author: rlyerly
 */

using namespace std;

#include "program_options.h"
#include "add_scheduler_calls_common.h"

/*
 * Initialize and parse program options for this pass.
 */
ProgramOptions::ProgramOptions(int argc, char** argv)
{
	desc.add_options()
		(SCHED_HEADER, po::value<string>(), "set heterogeneous scheduler library header location")
		(FEATURES_FOLDER, po::value<string>(), "specify the folder that contains the feature files")
	;

	po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	po::notify(vm);
}

/*
 * Return a string representing the default or user-specified location of the
 * heterogeneous scheduling library header.
 */
string ProgramOptions::getSchedulerHeaderLocation()
{
	if(vm.count(SCHED_HEADER))
		return vm[SCHED_HEADER].as<string>();
	else
		return DEF_SCHED_HEADER;
}

/*
 * Store, in the specified string, the specified features folder.  Additionally,
 * return true if this is the default folder, or false otherwise.
 */
bool ProgramOptions::getFeaturesFolder(string& buf)
{
	if(vm.count(FEATURES_FOLDER))
	{
		string retval = vm[FEATURES_FOLDER].as<string>();
		if(retval[retval.length()] != '/')
			buf = retval + "/";
		else
			buf = retval;
		return false;
	}
	else
	{
		buf = DEF_FEATURES_FOLDER;
		return true;
	}
}

