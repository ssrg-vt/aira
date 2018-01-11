/*
 * call_graph.cpp
 *
 *  Created on: Apr 15, 2013
 *      Author: rlyerly
 */

/* ROSE headers */
#include "rose.h"

/* call_graph headers */
#include "common.h"
#include "kernel_interface_common.h"
#include "call_graph.h"

/*
 * Construct the call graph wrapper.  Wrap each function called in a kernel
 * interface object and tell it what other functions it calls.
 */
CallGraph::CallGraph(SgProject* p_project) :
	project(p_project),
	cgBuilder(p_project)
{
	//Generate the call-graph
	cgBuilder.buildCallGraph(builtinFilter()); //TODO define our own filter?
	callGraph = cgBuilder.getGraph();
	callGraphNodes = callGraph->computeNodeSet();

	//Wrap each graph node in a FunctionInterface object
	set<SgGraphNode*>::const_iterator nodeIt;
	SgFunctionDeclaration* curFuncDecl;
	FunctionInterface* curFunc;
	for(nodeIt = callGraphNodes.begin(); nodeIt != callGraphNodes.end(); nodeIt++)
	{
		curFuncDecl = isSgFunctionDeclaration((*nodeIt)->get_SgNode());
		ROSE_ASSERT(curFuncDecl);

		curFuncDecl = isSgFunctionDeclaration(curFuncDecl->get_definingDeclaration());
		if(curFuncDecl)
		{
			curFunc = new FunctionInterface(curFuncDecl, *nodeIt);
			functionsToAnalyze.insert(curFunc);
		}
	}

	//Tell each FunctionInterface object what functions it calls
	set<FunctionInterface*>::const_iterator funcIt;
	set<SgDirectedGraphEdge*> edges;
	set<SgDirectedGraphEdge*>::const_iterator edgeIt;
	SgGraphNode* curNode;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
	{
		set<FunctionInterface*> funcsCalled;
		curNode = (*funcIt)->getGraphNode();
		edges = callGraph->computeEdgeSetOut(curNode);
		for(edgeIt = edges.begin(); edgeIt != edges.end(); edgeIt++)
		{
			curFunc = find((*edgeIt)->get_to());
			if(curFunc)
				funcsCalled.insert(curFunc);
		}
		(*funcIt)->storeFunctionsCalled(funcsCalled);
	}
}

/*
 * Delete all of the function interface objects allocated.
 */
CallGraph::~CallGraph()
{
	set<FunctionInterface*>::iterator funcIt;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
		delete *funcIt;
}

/*
 * Analyze each object and prepare it for annotations.
 */
void CallGraph::analyzeFunctions()
{
	set<FunctionInterface*>::const_iterator funcIt;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
		if((*funcIt)->getStatus() == NOT_ANALYZED)
			(*funcIt)->analyze();
}

/*
 * Annotate each object with pragmas.
 */
void CallGraph::annotateFunctions()
{
	set<FunctionInterface*>::const_iterator funcIt;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
		(*funcIt)->annotate();
}

/*
 * Find the FunctionInterface object corresponding to the passed graph node.
 */
FunctionInterface* CallGraph::find(SgGraphNode* node)
{
	set<FunctionInterface*>::const_iterator nodeIt;
	for(nodeIt = functionsToAnalyze.begin(); nodeIt != functionsToAnalyze.end(); nodeIt++)
	{
		if((*nodeIt)->getGraphNode() == node)
			return *nodeIt;
	}
	return NULL;
}
