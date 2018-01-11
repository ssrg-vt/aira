/*
 * call_graph.h
 *
 * Call graph wrapper which
 *
 * Created on: Apr 15, 2013
 * Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef CALL_GRAPH_H_
#define CALL_GRAPH_H_

#include "CallGraph.h"
#include "function_interface.h"

class CallGraph {
public:
	CallGraph(SgProject*);
	~CallGraph();
	void analyzeFunctions();
	void annotateFunctions();

private:
	SgProject* project;
	CallGraphBuilder cgBuilder;
	SgIncidenceDirectedGraph* callGraph;
	set<SgGraphNode*> callGraphNodes;
	set<FunctionInterface*> functionsToAnalyze;

	FunctionInterface* find(SgGraphNode*);
};

#endif /* CALL_GRAPH_H_ */
