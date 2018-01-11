/*
 * kernel_interface.cpp
 *
 *  Created on: Apr 9, 2013
 *      Author: rlyerly
 */

/* ROSE headers */
#include "rose.h"

/* Kernel interface headers */
#include "common.h"
#include "kernel_interface_common.h"
#include "call_graph.h"

int main(int argc, char** argv)
{
	//TODO command-line options

	//Initialize the project
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project);
	AstTests::runAllTests(project); //TODO switch on/off with command-line args

	//Analyze/annotate functions with their interface
	CallGraph cg(project);
	cg.analyzeFunctions();
	cg.annotateFunctions();

	//Cleanup
	return backend(project);
}
