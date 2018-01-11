/*
 * function_interface.h
 *
 *  Created on: Apr 9, 2013
 *      Author: rlyerly
 */

#ifndef FUNCTION_INTERFACE_H_
#define FUNCTION_INTERFACE_H_

class FunctionInterface {
public:
	FunctionInterface(SgFunctionDeclaration*, SgGraphNode*);
	void storeFunctionsCalled(set<FunctionInterface*>);
	enum analysisStatus getStatus();
	SgGraphNode* getGraphNode();
	void analyze();
	void annotate();

	SgFunctionDeclaration* getDeclaration();
	set<SgInitializedName*> getGlobalInputs();
	set<SgInitializedName*> getGlobalOutputs();
	set<FunctionInterface*> getCalledFunctions();

	friend class FunctionTraversal;
private:
	SgFunctionDeclaration* function;
	SgGraphNode* node;
	enum analysisStatus status;
	bool shouldAnnotate;

	set<SgInitializedName*> inputs;
	set<SgInitializedName*> outputs;
	set<SgInitializedName*> globalInputs;
	set<SgInitializedName*> globalOutputs;
	set<FunctionInterface*> calledFunctions;
	set<SgClassType*> classesNeeded;

	bool addInput(SgInitializedName*);
	bool addOutput(SgInitializedName*);
	bool addGlobalInput(SgInitializedName*);
	bool addGlobalOutput(SgInitializedName*);
	bool addClassNeeded(SgClassType*);
	void combineGlobalInputs(set<SgInitializedName*>);
	void combineGlobalOutputs(set<SgInitializedName*>);
	void combineCalledFunctions(set<FunctionInterface*>);
};

#endif /* FUNCTION_INTERFACE_H_ */
