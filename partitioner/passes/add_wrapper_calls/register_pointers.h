/*
 * register_pointers.h
 *
 *  Created on: Apr 23, 2013
 *      Author: rlyerly
 */

#ifndef REGISTER_POINTERS_H_
#define REGISTER_POINTERS_H_

/* TODO refactor into subclasses */
class RegisterPointers {
public:
	RegisterPointers(SgExpression* p_expression);
	RegisterPointers(SgInitializedName* p_varname, bool p_isGlobal = false);
	bool addRegUnregCalls();

	static void initialize();

private:
	SgExpression* expression;
	SgInitializedName* varName;
	SgVariableSymbol* varSymbol;
	bool isGlobal;
	bool definedInSystemHeader;
	bool compilerGenerated;
	bool addrUsedInIO;

	static set<string> headerFiles;
	static set<string> compilerVars;
	static set<string> functions;

	static void initializeSystemHeaderSet();
	static void initializeCompilerVarsSet();
	static void initializeFuncSet();

	bool isDefinedInSystemHeaders(SgInitializedName* var);
	bool isCompilerGenerated(SgSymbol* var);
	bool isAddrTakenInIrrelevantFunc(SgVarRefExp* expr);
};

#endif /* REGISTER_POINTERS_H_ */
