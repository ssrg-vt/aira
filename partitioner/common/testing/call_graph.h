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

class CallGraph {
public:
	CallGraph(SgProject*);
	set<SgGraphNode*> getGraphNodes();
	void setFunctionsToAnalyze(set<AnalysisObject*>);
	void analyzeFunctions();
	void annotateFunctions();

private:
	SgProject* project;
	CallGraphBuilder callgraphBuilder;
	SgIncidenceDirectedGraph* callGraph;
	set<SgGraphNode*> callGraphNodes;
	set<AnalysisObject*> functionsToAnalyze;

	AnalysisObject* find(SgGraphNode*);
};

#endif /* CALL_GRAPH_H_ */
