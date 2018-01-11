/*
 * call_graph.h
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

#ifndef CALL_GRAPH_H_
#define CALL_GRAPH_H_

#include "CallGraph.h"
#include "analyze_function.h"

class CallGraph {
public:
	CallGraph(SgProject*);
	~CallGraph();
	static void initializeWhitelist();
	bool annotateFunctions();
	void saveCallGraph();

	static const char* whitelisted[];

private:
	SgProject* project;
	CallGraphBuilder cgBuilder;
	SgIncidenceDirectedGraph* callGraph;
	set<SgGraphNode*> callGraphNodes;
	set<AnalyzeFunction*> functionsToAnalyze;
	set<SgFunctionDeclaration*> whitelist;

	AnalyzeFunction* find(SgGraphNode* graphNode);
};

#endif /* CALL_GRAPH_H_ */
