/*
 * function_traversal.h
 *
 *  Created on: Jan 22, 2013
 *      Author: rlyerly
 */

#ifndef FUNCTION_TRAVERSAL_H_
#define FUNCTION_TRAVERSAL_H_

class FunctionTraversal : public AstSimpleProcessing
{
public:
	FunctionTraversal(AnalyzeFunction*);
	virtual void visit(SgNode*);

	static void initializeWhitelist();
private:
	AnalyzeFunction* function;
	Rose_STL_Container<SgNode*> pragmas;

	void checkDynamicMemory(SgExpression* expr);
	void checkClass(SgVariableSymbol* varSymbol, SgClassType* type);
	bool isWhitelisted(SgFunctionDeclaration* func);

	static set<string> whitelistedFunctions;
};

#endif /* FUNCTION_TRAVERSAL_H_ */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
