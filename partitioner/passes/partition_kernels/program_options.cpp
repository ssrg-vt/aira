/*
 * program_options.cpp
 *
 *  Created on: May 8, 2013
 *      Author: rlyerly
 */

using namespace std;

#include "program_options.h"

namespace po = boost::program_options;

ProgramOptions::ProgramOptions(int argc, char** argv)
{
	desc.add_options()
			("mpiHeader", po::value<string>(), "set MPI header location")
			("mpiOutput", po::value<string>(), "set MPI partition output file")
			("mpiName", po::value<string>(), "set MPI partition name")
			("gpuHeader", po::value<string>(), "set GPU header location")
			("gpuErrorHeader", po::value<string>(), "set GPU error-handling header location")
			("gpuOutput", po::value<string>(), "set GPU partition output file")
			("gpuName", po::value<string>(), "set GPU partition name")
	;

	po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	po::notify(vm);
}

string ProgramOptions::getMpiHeaderLocation()
{
	if(vm.count("mpiHeader"))
		return vm["mpiHeader"].as<string>();
	else
		return "";
}

string ProgramOptions::getMpiOutputFile()
{
	if(vm.count("mpiOutput"))
		return vm["mpiOutput"].as<string>();
	else
		return "";
}

string ProgramOptions::getMpiPartitionName()
{
	if(vm.count("mpiName"))
		return vm["mpiName"].as<string>();
	else
		return "";
}

string ProgramOptions::getGpuHeaderLocation()
{
	if(vm.count("gpuHeader"))
		return vm["gpuHeader"].as<string>();
	else
		return "";
}

string ProgramOptions::getGpuErrorHeaderLocation()
{
	if(vm.count("gpuErrorHeader"))
		return vm["gpuErrorHeader"].as<string>();
	else
		return "";
}

string ProgramOptions::getGpuOutputFile()
{
	if(vm.count("gpuOutput"))
		return vm["gpuOutput"].as<string>();
	else
		return "";
}

string ProgramOptions::getGpuPartitionName()
{
	if(vm.count("gpuName"))
		return vm["gpuName"].as<string>();
	else
		return "";
}
