/*
 * analyze_function.h
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

#ifndef ANALYZE_FUNCTION_H_
#define ANALYZE_FUNCTION_H_

#include "common.h"

class AnalyzeFunction {
public:
	AnalyzeFunction(SgGraphNode* p_node, SgFunctionDeclaration* p_function);
	void storeFunctionsCalled(set<AnalyzeFunction*>);

	bool analyze();
	bool annotate();

	enum analysisStatus getStatus();
	SgGraphNode* getGraphNode();
	bool useTilera();
	bool useGpu();
	string getFuncName();

private:
	set<AnalyzeFunction*> calledFunctions;
	SgGraphNode* node;
	SgFunctionDeclaration* function;
	enum analysisStatus status;

	//Sets used to cut down on verbosity of output messages by storing
	//previously seen variables & functions
	set<SgVariableSymbol*> incompatibleVars;
	set<SgFunctionDeclaration*> whitelistedFuncs;
	set<SgFunctionDeclaration*> undefinedFuncs;
	set<string> dynamicMemFuncs;

	//Architectures
	bool shouldAnnotate;
	bool mpi;
	bool gpu;

	//Analysis flags
	bool callsUndefinedFunctions;
	bool usesHighOrderPointers; //Pointers of 3+ dimensions
	bool dynamicMem;
	bool usesUndefinedStruct;
	bool usesStructWithPointers;
	bool usesStructWithStruct;

	friend class FunctionTraversal;
};

#endif /* ANALYZE_FUNCTION_H_ */
