/*
 * function_traversal.h
 *
 *  Created on: Jan 22, 2013
 *      Author: rlyerly
 */

#ifndef FUNCTION_TRAVERSAL_H_
#define FUNCTION_TRAVERSAL_H_

class FunctionTraversal : public AstPrePostProcessing
{
public:
	FunctionTraversal(FunctionDeclaration* function);

	virtual void preOrderVisit(SgNode* node);
	virtual void postOrderVisit(SgNode* node);

	bool callsUndefinedFunctions();

private:
	FunctionDeclaration* function;

	bool undefinedFunctionsCalled;
};

#endif /* FUNCTION_TRAVERSAL_H_ */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
