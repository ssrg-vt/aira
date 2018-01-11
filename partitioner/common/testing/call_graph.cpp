/*
 * call_graph.cpp
 *
 * Created on: Apr 15, 2013
 * Author: Rob Lyerly <rlyerly@vt.edu>
 */

/* ROSE headers */
#include "rose.h"

/* call_graph headers */
#include "common.h"
#include "analysis_object.h"
#include "call_graph.h"

/*
 * Construct the call graph wrapper
 */
CallGraph::CallGraph(SgProject* p_project) :
	project(p_project),
	callgraphBuilder(p_project)
{
	//Generate the call-graph
	callgraphBuilder.buildCallGraph(builtinFilter()); //TODO define our own filter?
	callGraph = callgraphBuilder.getGraph();
	callGraphNodes = callGraph->computeNodeSet();
}

/*
 * Return the set of nodes in the call graph
 */
set<SgGraphNode*> CallGraph::getGraphNodes()
{
	return callGraphNodes;
}

/*
 * Set this callgraph's objects to analyze
 */
void CallGraph::setFunctionsToAnalyze(set<AnalysisObject*> p_functionsToAnalyze)
{
	functionsToAnalyze = p_functionsToAnalyze;

	//Tell each analysis object what functions it calls
	set<AnalysisObject*>::const_iterator funcIt;
	set<SgDirectedGraphEdge*> edges;
	set<SgDirectedGraphEdge*>::const_iterator edgeIt;
	SgGraphNode* curNode;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
	{
		set<AnalysisObject*> funcsCalled;
		curNode = (*funcIt)->getGraphNode();
		edges = callGraph->computeEdgeSetOut(curNode);
		for(edgeIt = edges.begin(); edgeIt != edges.end(); edgeIt++)
			funcsCalled.insert(find((*edgeIt)->get_to()));
		(*funcIt)->setFunctionsCalled(funcsCalled);
	}
}

/*
 * Analyze each object and prepare it for annotation
 */
void CallGraph::analyzeFunctions()
{
	set<AnalysisObject*>::const_iterator funcIt;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
		if((*funcIt)->getStatus() == NOT_ANALYZED)
			(*funcIt)->analyze();
}

/*
 * Annotate each object with pragmas
 */
void CallGraph::annotateFunctions()
{
	set<AnalysisObject*>::const_iterator funcIt;
	for(funcIt = functionsToAnalyze.begin(); funcIt != functionsToAnalyze.end(); funcIt++)
		(*funcIt)->annotate();
}

/*
 * Find the FunctionInterface object corresponding to the passed graph node
 */
FunctionInterface* CallGraph::find(SgGraphNode* node)
{
	set<AnalysisObject*>::const_iterator nodeIt;
	for(nodeIt = functionsToAnalyze.begin(); nodeIt != functionsToAnalyze.end(); nodeIt++)
	{
		if((*nodeIt)->getGraphNode() == node)
			return *nodeIt;
	}
	return NULL;
}
