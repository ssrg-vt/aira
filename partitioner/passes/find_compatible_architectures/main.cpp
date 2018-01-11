/*
 * find_compatible_architectures.cpp
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

/* ROSE headers */
#include "rose.h"

/* Architecture Finder headers */
#include "call_graph.h"
#include "architecture_finder_common.h"

int numTabs = 0;

int main(int argc, char** argv)
{
	//TODO command-line options

	//Setup the project
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project);
	AstTests::runAllTests(project); //TODO switch on/off with command-line args

	if(project->get_fileList().size() < 1)
	{
		ERROR(TOOL, "No input files");
		return NO_INPUT_FILES;
	}

	//Generate the call-graph
	CallGraph callGraph(project);
	callGraph.initializeWhitelist();	//TODO allow adding of white-listed
										//from command line
	if(false) //TODO allow based on command-line args
	{
		generateDOT(*project);
		callGraph.saveCallGraph();
	}

	//Search and annotate functions with compatible architectures
	callGraph.annotateFunctions();

	return backend(project);
}
