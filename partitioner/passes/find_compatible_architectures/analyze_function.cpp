/*
 * analyze_function.cpp
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "architecture_finder_common.h"
#include "analyze_function.h"
#include "function_traversal.h"
#include "pragma_handling.h"

AnalyzeFunction::AnalyzeFunction(SgGraphNode* p_node, SgFunctionDeclaration* p_function) :
	node(p_node),
	function(p_function),
	status(NOT_ANALYZED),
	shouldAnnotate(false),
	mpi(true),
	gpu(true),
	callsUndefinedFunctions(false),
	usesHighOrderPointers(false),
	dynamicMem(false),
	usesUndefinedStruct(false),
	usesStructWithPointers(false),
	usesStructWithStruct(false)
{
	//See if this function contains OpenMP pragmas - if so, annotate
	Rose_STL_Container<SgNode*> pragmas = querySubTree(p_function, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt = pragmas.begin();
	SgPragmaDeclaration* pragma;
	for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
	{
		pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma != NULL);

		if(extractPragmaKeyword(pragma) == "omp")
		{
			shouldAnnotate = true;
			break;
		}
	}
}

void AnalyzeFunction::storeFunctionsCalled(set<AnalyzeFunction*> funcsCalled)
{
	calledFunctions = funcsCalled;
}

/*
 * Analyze the function for its compatibility on different architectures.
 */
bool AnalyzeFunction::analyze()
{
	status = IN_PROGRESS;

	string msg = "Analyzing function: " + NAME(function);
	DEBUG(TOOL, msg);

	//First, analyze all nodes called by this node
	std::set<AnalyzeFunction*>::const_iterator funcIt;
	for(funcIt = calledFunctions.begin(); funcIt != calledFunctions.end(); funcIt++)
	{
		if((*funcIt)->getStatus() == NOT_ANALYZED)
		{
			numTabs++;
			(*funcIt)->analyze();
			numTabs--;
		}
	}

	//Analyze this node
	FunctionTraversal ft(this);
	ft.traverse(function, preorder);

	//Next, turn off architecture compatibility based on flags
	if(callsUndefinedFunctions)
		mpi = gpu = false;
	if(usesHighOrderPointers)
		mpi = gpu = false;
	if(dynamicMem)
		gpu = false;
	if(usesUndefinedStruct || usesStructWithPointers || usesStructWithStruct)
		mpi = gpu = false;

	//Finally, incorporate analysis from sub-calls.  If sub-calls are in progress,
	//there is a cycle in the call-graph.  Consequently, these cycles all must have
	//the same compatibility.
	for(funcIt = calledFunctions.begin(); funcIt != calledFunctions.end(); funcIt++)
	{
		if(!(*funcIt)->useTilera())
			mpi = false;
		if(!(*funcIt)->useGpu())
			gpu = false;
	}

	status = ANALYZED;
	return true;
}

/*
 * Based on analysis, add appropriate architecture annotations.
 */
bool AnalyzeFunction::annotate()
{
	if(!shouldAnnotate)
		return false;

	string msg = "Annotating function " + NAME(function);
	DEBUG(TOOL, msg);

	//For the first go-around, we'll assume a constant hardware setup
	//TODO make hardware setup modular
	set<string> hardware;

	hardware.insert("x86");
	if(mpi)
		hardware.insert("mpi");
	if(gpu)
		hardware.insert("gpu");

	//List what this function does
	if(callsUndefinedFunctions)
		DEBUG(TOOL, "\t-> calls undefined functions");
	if(usesHighOrderPointers)
		DEBUG(TOOL, "\t-> uses higher-dimension (3+) pointers");
	if(dynamicMem)
		DEBUG(TOOL, "\t-> uses dynamic memory");
	if(usesUndefinedStruct)
		DEBUG(TOOL, "\t-> uses an invisible/undeclared struct");
	if(usesStructWithPointers)
		DEBUG(TOOL, "\t-> uses a struct that has pointer fields");
	if(usesStructWithStruct)
		DEBUG(TOOL, "\t-> uses a struct that has an internal struct field");

	SgPragmaDeclaration* pragmaDecl = PragmaBuilder::buildPragma(COMPATIBLE_ARCH, hardware,
			function->get_scope());
	insertStatement(function, pragmaDecl);

	return true;
}

/*
 * Returns the analysis status of this node.
 */
enum analysisStatus AnalyzeFunction::getStatus()
{
	return status;
}

/*
 * Get the graph node corresponding to this function.
 */
SgGraphNode* AnalyzeFunction::getGraphNode()
{
	return node;
}

/*
 * Return whether or not this function is compatible with Tilera.
 */
bool AnalyzeFunction::useTilera()
{
	return mpi;
}

/*
 * Return whether or not this function is compatible with the GPU.
 */
bool AnalyzeFunction::useGpu()
{
	return gpu;
}

/*
 * Return the name of the function encapsulated by this object.
 */
string AnalyzeFunction::getFuncName()
{
	return NAME(function);
}
