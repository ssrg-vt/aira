/*
 * function_call_updater.h
 *
 *  Created on: Apr 22, 2013
 *      Author: rlyerly
 */

#ifndef FUNCTION_CALL_UPDATER_H_
#define FUNCTION_CALL_UPDATER_H_

class FunctionCallUpdater {
public:
	FunctionCallUpdater(SgFunctionDeclaration* p_function, Rose_STL_Container<SgNode*>& funcCalls);
	void updateDeclaration();
	void updateSites();

	static void insertInitWrapperCall(SgFunctionDeclaration*);
private:
	SgFunctionDeclaration* function;
	set<SgFunctionCallExp*> callSites;
	set<SgVariableDeclaration*> registeredVars;
};

#endif /* FUNCTION_CALL_UPDATER_H_ */
