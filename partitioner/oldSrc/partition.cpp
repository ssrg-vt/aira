/*
 * Partitioner built on the Rose Compiler Infrastructure
 *
 * Re-factors input program to partition kernels based on user-pragmas and internal analysis.
 */

/* Rose headers - must come first for PCH to work */
#include "rose.h"
#include <CallGraph.h>

/* Standard library headers */
#include <iostream>
#include <set>

/* Partitioner headers */
#include "program_options.h"
#include "function_declaration.h"
#include "function_call.h"
#include "create_partition.h"
#include "pragma_definitions.h"

#define SUCCESS 0
#define FAILURE 1

using namespace std;
using namespace StringUtility;
using namespace SageBuilder;
using namespace SageInterface;
using namespace NodeQuery;

int main(int argc, char** argv)
{
	//Parse command-line or configuration file arguments
	//TODO configuration files
	ProgramOptions myArgs(argc, argv);

	//Print help
	if(myArgs.printHelp())
	{
		cout << "This is the help.  It will be more substantial once there is stuff to print." << endl;
		return SUCCESS;
	}

	//Build AST & check
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project != NULL);
	if(myArgs.runASTConsistencyTests())
	{
		cout << "Running consistency tests...";
		AstTests::runAllTests(project);
		cout << "finished" << endl;
	}

	//Generate call-graph
	if(myArgs.printCallGraph()) {
		if(project->get_fileList().size() >= 1) {
			CallGraphBuilder cgbuilder(project);
			cgbuilder.buildCallGraph(builtinFilter());

			AstDOTGeneration dotgen;
			SgFilePtrList file_list = project->get_fileList();
			string firstFileName = stripPathFromFileName(file_list[0]->getFileName());
			dotgen.writeIncidenceGraphToDOTFile(cgbuilder.getGraph(), firstFileName + "_callgraph.dot");
		}
	}

	//Generate AST graph
	if(myArgs.printAST())
		generateDOT(*project);

	//Generate full AST graph
	if(myArgs.printFullAST())
		generatePDF(*project);

	// TODO Create partitions for hardware specified in the configuration file
	MPIPartition::setMPIHeader(myArgs.getMPIHeader());
	Partition *tilera = new MPIPartition("Tilera");

	//Insert code to so that it is an MPI program
	insertHeader(myArgs.getMPIHeader(), PreprocessingInfo::after, true,
			getFirstGlobalScope(project));
	//TODO add any necessary MPI code

	//Partition indicated functions (i.e. functions annotated with pragmas)
	Rose_STL_Container<SgNode*> pragmas = querySubTree(project, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt;
	Rose_STL_Container<SgNode*> functionCallList;
	Rose_STL_Container<SgNode*>::const_iterator functionCallIt;
	for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
	{
		SgPragmaDeclaration* pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma != NULL);

		if(extractPragmaKeyword(pragma) == POPCORN_PRAGMA)
		{
			//Check to make sure the next statement is a function declaration
			SgFunctionDeclaration* annotatedFunction = isSgFunctionDeclaration(getNextStatement(pragma));
			if(!annotatedFunction)
			{
				cerr << "Warning: found partitioning pragma before a statement that was not a function "
						<< "declaration.  Continuing to next pragma..." << endl;
				continue;
			}

			//If we cannot partition the function, continue to next pragma
			FunctionDeclaration functionToPartition(annotatedFunction, true);
			if(!functionToPartition.canPartition())
			{
				functionToPartition.printPartitioningStatus();
				continue;
			}

			//Partition the function
			if(pragma->get_pragma()->get_pragma().find("tilera") != string::npos)
				functionToPartition.partition(tilera);
			else
			{
				cout << "Unknown partition: " << pragma->get_pragma()->get_pragma() << endl;
				continue;
			}

			//Get all function calls associated with this function and update their arguments accordingly
			functionCallList = querySubTree(project, V_SgFunctionCallExp);
			for(functionCallIt = functionCallList.begin(); functionCallIt != functionCallList.end();
					functionCallIt++)
			{
				SgFunctionCallExp* functionCallExp = isSgFunctionCallExp(*functionCallIt);
				ROSE_ASSERT(functionCallExp != NULL);

				if(functionCallExp->getAssociatedFunctionDeclaration()->get_definingDeclaration() !=
						annotatedFunction)
					continue;

				FunctionCall functionCall(functionCallExp);
				functionCall.updateCallSite();
			}
		}
	}

	//Finalize and cleanup partitions
	tilera->finalize();
	delete tilera;

	//Generate source code from updated AST
	return backend(project);
}
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
