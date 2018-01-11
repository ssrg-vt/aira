/*
 * function_traversal.h
 *
 *  Created on: Apr 9, 2013
 *      Author: rlyerly
 */

#ifndef FUNCTION_TRAVERSAL_H_
#define FUNCTION_TRAVERSAL_H_

class FunctionTraversal : public AstSimpleProcessing {
public:
	FunctionTraversal(FunctionInterface*);
	virtual void visit(SgNode* node);

	static void checkVariableType(SgType*, FunctionInterface*);
private:
	FunctionInterface* function;
	bool isPure;
	string functionName;

	void checkGlobalVars(SgVarRefExp*);
	void checkSideEffects(SgBinaryOp*);
	void checkFunctionCalls(SgFunctionCallExp*);
	void checkVariableType(SgType*);
};

#endif /* FUNCTION_TRAVERSAL_H_ */
