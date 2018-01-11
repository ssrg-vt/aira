/*
 * main.cpp
 *
 *  Created on: Apr 17, 2013
 *      Author: rlyerly
 */

//TODO this file needs re-factoring, it is extremely messy and maybe unmaintainable

/* ROSE headers */
#include "rose.h"

/* Other headers */
#include "common.h"
#include "add_wrapper_calls_common.h"
#include "register_vars.h"
#include "register_pointers.h"
#include "pragma_handling.h"
#include "function_call_updater.h"
#include "program_options.h"

/*
 * Maintain set of all global variables needed so we can register/unregister
 * them
 */
void saveGlobalVariables(PragmaParser* pp, set<SgInitializedName*>& globalVars, SgScopeStatement* scope)
{
	set<string> names = pp->getNames();
	set<string>::const_iterator nameIt;
	for(nameIt = names.begin(); nameIt != names.end(); nameIt++)
	{
		SgName curVarName(*nameIt);
		if(curVarName.getString().rfind('*') != string::npos)
			curVarName.getString().resize(curVarName.getString().length() - 1);
		SgVariableSymbol* varSymbol = lookupVariableSymbolInParentScopes(curVarName, scope);
		ROSE_ASSERT(varSymbol);
		if(globalVars.find(varSymbol->get_declaration()) == globalVars.end())
			globalVars.insert(varSymbol->get_declaration());
	}
}

int main(int argc, char** argv)
{
	//Parse command-line options
	ProgramOptions po(argc, argv);

	//Initialize the AST
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project);
	AstTests::runAllTests(project); //TODO switch on/off with command-line args

	//Insert MM-wrapper header file
	string mmHeader = po.getMMWrapperHeaderLocation();
	insertHeader(mmHeader, PreprocessingInfo::after, false, getGlobalScope(findMain(project)));

	//Initialize set of system headers & compiler generated vars
	RegisterPointers::initialize();

	//Add calls to registers sizes of static variables
	RegisterVars rv(project, mmHeader);
	rv.registerStaticVars();

	//Used to accumulate all global variables
	set<SgInitializedName*> globalVars;

	//Add wrapper calls to each function/sub-function so that all pointers are registered
	Rose_STL_Container<SgNode*> pragmas = querySubTree(project, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt;
	Rose_STL_Container<SgNode*> funcCalls = querySubTree(project, V_SgFunctionCallExp);
	SgPragmaDeclaration* pragma;
	SgStatement* stmt;
	SgFunctionDeclaration* funcDecl;
	for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
	{
		pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma);

		PragmaParser pp(pragma);
		if(pp.isPopcornPragma() && pp.getPragmaType() == PARTITIONED)
		{
			//Get function declaration
			stmt = getNextStatement(pragma);
			while(!isSgFunctionDeclaration(stmt))
				stmt = getNextStatement(stmt);
			funcDecl = isSgFunctionDeclaration(stmt);
			ROSE_ASSERT(funcDecl);
			Pragmas pragmas(funcDecl);

			//Save global variables
			saveGlobalVariables(pragmas.getGlobalInputs(), globalVars,
					pragmas.getFunction()->get_scope());
			saveGlobalVariables(pragmas.getGlobalOutputs(), globalVars,
					pragmas.getFunction()->get_scope());

			//Update call sites
			//TODO I don't think we need this anymore, since all sizes are
			//handled through the mm_wrapper interface
			//FunctionCallUpdater fcu(funcDecl, funcCalls);
			//fcu.updateDeclaration();
			//fcu.updateSites();
		}
	}

	//Register/unregister global variables
	RegisterVars::registerGlobalVars(globalVars);

	//Add call to initialize wrapper
	SgFunctionDeclaration* main = findMain(project);
	ROSE_ASSERT(main);
	FunctionCallUpdater::insertInitWrapperCall(main);

	return backend(project);
}
