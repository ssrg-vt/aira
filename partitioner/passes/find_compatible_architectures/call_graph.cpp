/*
 * call_graph.cpp
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

/* ROSE headers */
#include "rose.h"

/* call_graph headers */
#include "call_graph.h"

#include "function_traversal.h"

/*
 * Generates the call-graph and wraps all functions in an "AnalyzeFunction"
 * wrapper object.
 */
CallGraph::CallGraph(SgProject* p_project) :
	project(p_project),
	cgBuilder(p_project)
{
	//Generate the call-graph
	cgBuilder.buildCallGraph(builtinFilter()); //TODO define our own filter?
	callGraph = cgBuilder.getGraph();
	callGraphNodes = callGraph->computeNodeSet();

	//Initialize set of whitelisted functions
	FunctionTraversal::initializeWhitelist();

	//Wrap each graph node in an AnalyzeFunction object
	set<SgGraphNode*>::const_iterator nodeIt;
	SgFunctionDeclaration* curFuncDecl;
	AnalyzeFunction* curFunc;
	for(nodeIt = callGraphNodes.begin(); nodeIt != callGraphNodes.end(); nodeIt++)
	{
		curFuncDecl = isSgFunctionDeclaration((*nodeIt)->get_SgNode());
		ROSE_ASSERT(curFuncDecl);

		curFuncDecl = isSgFunctionDeclaration(curFuncDecl->get_definingDeclaration());
		if(curFuncDecl)
		{
			curFunc = new AnalyzeFunction(*nodeIt, curFuncDecl);
			functionsToAnalyze.insert(curFunc);
		}
	}

	//Tell each AnalyzedFunction object what functions it calls
	set<AnalyzeFunction*>::const_iterator funcIt;
	set<SgDirectedGraphEdge*> edges;
	set<SgDirectedGraphEdge*>::const_iterator edgeIt;
	SgGraphNode* curNode;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
	{
		set<AnalyzeFunction*> funcsCalled;
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

CallGraph::~CallGraph()
{
	set<AnalyzeFunction*>::iterator funcIt;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
		delete *funcIt;
}

/*
 * Analyze each function in the call-graph and annotate with pragmas specifying
 * compatible architectures.
 */
bool CallGraph::annotateFunctions()
{
	//First, analyze the functions
	set<AnalyzeFunction*>::const_iterator nodeIt;
	for(nodeIt = functionsToAnalyze.begin(); nodeIt != functionsToAnalyze.end(); nodeIt++)
	{
		if((*nodeIt)->getStatus() == NOT_ANALYZED)
			(*nodeIt)->analyze();
	}

	//Second, annotate with pragmas for compatible architectures
	for(nodeIt = functionsToAnalyze.begin(); nodeIt != functionsToAnalyze.end(); nodeIt++)
		(*nodeIt)->annotate();

	return true;
}

/*
 * Save the call-graph into a .dot file.
 */
void CallGraph::saveCallGraph()
{
	AstDOTGeneration dotGen;
	SgFilePtrList fileList = project->get_fileList();
	string fileName = stripPathFromFileName(fileList[0]->getFileName());
	dotGen.writeIncidenceGraphToDOTFile(callGraph, fileName + "_cg.dot");
}

/*
 * Find the wrapper for the passed function declaration, or return null if it
 * doesn't exist.
 */
AnalyzeFunction* CallGraph::find(SgGraphNode* node)
{
	set<AnalyzeFunction*>::const_iterator nodeIt;
	for(nodeIt = functionsToAnalyze.begin(); nodeIt != functionsToAnalyze.end(); nodeIt++)
	{
		if((*nodeIt)->getGraphNode() == node)
			return *nodeIt;
	}
	return NULL;
}

/*
 * Initialize the white-list of undefined functions that are allowed to be
 * called on a partition.
 */
void CallGraph::initializeWhitelist()
{
	//TODO
}
