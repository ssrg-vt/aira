/*
 * function_traversal.cpp
 *
 *  Created on: Jan 22, 2013
 *      Author: rlyerly
 */

#include "rose.h" // Must come first for PCH to work.

#include "analyze_function.h"
#include "function_traversal.h"
#include "common.h"
#include "architecture_finder_common.h"
#include "miscellaneous.h"

/*
 * Default constructor.
 */
FunctionTraversal::FunctionTraversal(AnalyzeFunction* function) :
		function(function)
{
	//Get all the pragmas inside this function, for later use
	pragmas = querySubTree(function->function, V_SgPragmaDeclaration);
}

/*
 * Analyze this node (in the function subtree) for specific characteristics.
 */
void FunctionTraversal::visit(SgNode* node)
{
	string msg;

	SgFunctionCallExp* funcCall = isSgFunctionCallExp(node);
	SgDeclarationStatement* funcDecl = NULL;
	SgVarRefExp* varRef = isSgVarRefExp(node);
	SgNewExp* newExp = isSgNewExp(node);
	SgDeleteExp* deleteExp = isSgDeleteExp(node);
	SgType* type = NULL;
	SgVariableSymbol* varSymbol = NULL;

	if(newExp || deleteExp)
	{
		SgExpression* passExp = newExp;
		if(deleteExp != NULL)
			passExp = deleteExp;

		if(passExp == newExp)
			msg = "\t-> Contains new expression";
		else
			msg = "\t-> Contains delete expression";
		DEBUG(TOOL, msg);

		checkDynamicMemory(passExp);
	}
	else if(funcCall)	//Check for undefined functions and C-Style dynamic memory management
	{
		funcDecl = isSgFunctionDeclaration(
						funcCall->getAssociatedFunctionDeclaration()->get_definingDeclaration());
		SgName name = NAME(funcCall->getAssociatedFunctionDeclaration());

		if(name == "malloc" || name == "free" || name == "calloc" || name == "realloc")
		{
			//C-style dynamic memory management
			if(function->dynamicMemFuncs.insert(name).second)
			{
				msg = "\t-> Contains call to dynamic memory function " + name;
				DEBUG(TOOL, msg);
			}
			checkDynamicMemory(funcCall);
		}
		else if(!funcDecl)
		{
			if(isWhitelisted(funcCall->getAssociatedFunctionDeclaration()))
			{
				//Whitelisted functions
				if(function->whitelistedFuncs.insert(
						funcCall->getAssociatedFunctionDeclaration()).second)
				{
					msg = "\t-> Contains call to white-listed function " + name;
					DEBUG(TOOL, msg);
				}
			}
			else
			{
				//Undefined functions
				if(function->undefinedFuncs.insert(
						funcCall->getAssociatedFunctionDeclaration()).second)
				{
					msg = "\t-> Contains call to undefined function " + name;
					DEBUG(TOOL, msg);
					function->callsUndefinedFunctions = true;
				}
			}
		}
	}
	else if(varRef)	//Check for higher-dimensional pointers & structs
	{
		int numDimensions = 0;
		SgType* baseType = NULL;
		type = varRef->get_type()->stripTypedefsAndModifiers();\
		Misc::getType(&type, &baseType, &numDimensions);
		if(baseType)
			type = baseType;

		if(numDimensions > 2)
		{
			varSymbol = varRef->get_symbol();
			if(function->incompatibleVars.insert(varSymbol).second)
			{
				msg = "\t-> Variable " + NAME(varSymbol)
						+ " is a higher-dimension (3+) pointer variable";
				DEBUG(TOOL, msg);
				function->usesHighOrderPointers = true;
			}
		}
		else if(isSgClassType(type))
			checkClass(varRef->get_symbol(), isSgClassType(type));
	}

	return;
}

/*
 * Check the dynamic memory expression (C or C++ style) against the following
 * criteria:
 * 1. This function has an OMP parallel block - if the memory ops are inside
 * 		block, then the function uses dynamic memory.  Otherwise, it doesn't
 * 2. This function doesn't have a parallel block, and therefore does use
 * 		dynamic memory inside the kernel
 */
void FunctionTraversal::checkDynamicMemory(SgExpression* expr)
{
	ROSE_ASSERT(expr != NULL);

	if(pragmas.size() == 0)
	{
		function->dynamicMem = true;
		return;
	}
	else
	{
		Rose_STL_Container<SgNode*>::const_iterator pragmaIt = pragmas.begin();
		SgPragmaDeclaration* pragma = NULL;
		SgStatement* stmt = NULL;
		for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
		{
			pragma = isSgPragmaDeclaration(*pragmaIt);
			ROSE_ASSERT(pragma != NULL);

			if(pragma->get_pragma()->get_pragma().find("omp parallel") != string::npos)
			{
				//Found an OMP parallel pragma - see if function call is within it's scope
				stmt = getNextStatement(pragma);

				if(stmt == getScope(stmt))
				{
					//Next statement is a scope statement, bottom up search
					SgScopeStatement* scope = getScope(expr);
					do
					{
						if(scope == stmt)
						{
							function->dynamicMem = true;
							break;
						}
						if(scope == getScope(getEnclosingStatement(scope)))
							break;
						else
							scope = getScope(getEnclosingStatement(scope));
					}
					while(scope != function->function->get_definition());
				}
				else if(getEnclosingStatement(expr) == stmt)
				{
					//Function call is within next statement
					function->dynamicMem = true;
					break;
				}
			}
		}
	}
}

/*
 * Check the user-defined struct or class for pointers.
 */
void FunctionTraversal::checkClass(SgVariableSymbol* varSymbol, SgClassType* type)
{
	string msg;

	//Get class declaration
	SgClassDeclaration* classDecl =	isSgClassDeclaration(type->get_declaration());
	classDecl = isSgClassDeclaration(classDecl->get_definingDeclaration());
	if(!classDecl)
	{
		if(function->incompatibleVars.insert(varSymbol).second)
		{
			msg = "\t-> Variable " + NAME(varSymbol) + " is an undefined struct/class";
			DEBUG(TOOL, msg);
			function->usesUndefinedStruct = true;
		}
		return;
	}
	SgClassDefinition* classDef = classDecl->get_definition();

	//Iterate through members, looking for pointers & structs
	SgDeclarationStatementPtrList& members = classDef->get_members();
	SgDeclarationStatementPtrList::const_iterator memberIt = members.begin();
	SgVariableDeclaration* varDecl = NULL;
	for(memberIt = members.begin(); memberIt != members.end(); memberIt++)
	{
		varDecl = isSgVariableDeclaration(*memberIt);
		if(varDecl) //TODO ROSE doesn't seem to differentiate between data + methods as members
		{
			SgType* varType = varDecl->get_definition()->get_type();
			SgType* baseType = NULL;
			int numDimensions = 0;
			Misc::getType(&varType, &baseType, &numDimensions);
			if(isSgPointerType(varType) && numDimensions > 2)
			{
				if(function->incompatibleVars.insert(varSymbol).second)
				{
					msg = "\t-> Variable " + NAME(varSymbol)
							+ " is a struct/class w/ higher-dimension pointer members";
					DEBUG(TOOL, msg);
					function->usesStructWithPointers = true;
				}
			}
			else if(isSgClassType(baseType))
			{
				if(function->incompatibleVars.insert(varSymbol).second)
				{
					msg = "\t-> Variable " + NAME(varSymbol)
							+ " is a struct/class w/ struct/class members";
					DEBUG(TOOL, msg);
					function->usesStructWithStruct = true;
				}
			}
		}
	}
}

/*
 * Check to see if the specified function is white-listed.
 */
bool FunctionTraversal::isWhitelisted(SgFunctionDeclaration* func)
{
	if(whitelistedFunctions.find(NAME(func)) != whitelistedFunctions.end())
		return true;
	else
		return false;
}

/*
 * We've only declared the set of white-listed functions, now we define it.
 */
set<string> FunctionTraversal::whitelistedFunctions;

/*
 * Initialize white-listed functions (undefined functions that are allowed
 * on a partition).
 *
 * TODO for now, assume if a function is white-listed, it is compatible on all
 * architectures, although that is definitely not the case
 */
void FunctionTraversal::initializeWhitelist()
{
	//Memory management functions
	whitelistedFunctions.insert("malloc");
	whitelistedFunctions.insert("calloc");
	whitelistedFunctions.insert("realloc");
	whitelistedFunctions.insert("free");

	//OpenMP functions
	whitelistedFunctions.insert("omp_set_num_threads");
	whitelistedFunctions.insert("omp_get_num_threads");
	whitelistedFunctions.insert("omp_get_max_threads");
	whitelistedFunctions.insert("omp_get_thread_num");
	whitelistedFunctions.insert("omp_get_thread_limit");
	whitelistedFunctions.insert("omp_get_num_procs");
	whitelistedFunctions.insert("omp_in_parallel");
	whitelistedFunctions.insert("omp_set_dynamic");
	whitelistedFunctions.insert("omp_get_dynamic");
	whitelistedFunctions.insert("omp_set_nested");
	whitelistedFunctions.insert("omp_get_nested");
	whitelistedFunctions.insert("omp_set_schedule");
	whitelistedFunctions.insert("omp_get_schedule");
	whitelistedFunctions.insert("omp_set_max_active_levels");
	whitelistedFunctions.insert("omp_get_max_active_levels");
	whitelistedFunctions.insert("omp_get_level");
	whitelistedFunctions.insert("omp_get_ancestor_thread_num");
	whitelistedFunctions.insert("omp_get_team_size");
	whitelistedFunctions.insert("omp_get_active_level");
	whitelistedFunctions.insert("omp_in_final");
	whitelistedFunctions.insert("omp_init_lock");
	whitelistedFunctions.insert("omp_destroy_lock");
	whitelistedFunctions.insert("omp_set_lock");
	whitelistedFunctions.insert("omp_unset_lock");
	whitelistedFunctions.insert("omp_test_lock");
	whitelistedFunctions.insert("omp_init_nest_lock");
	whitelistedFunctions.insert("omp_destroy_nest_lock");
	whitelistedFunctions.insert("omp_set_nest_lock");
	whitelistedFunctions.insert("omp_unset_nest_lock");
	whitelistedFunctions.insert("omp_test_nest_lock");
	whitelistedFunctions.insert("omp_get_wtime");
	whitelistedFunctions.insert("omp_get_wtick");

	//Math functions
	whitelistedFunctions.insert("abs");
	whitelistedFunctions.insert("cos");
	whitelistedFunctions.insert("cosf");
	whitelistedFunctions.insert("sin");
	whitelistedFunctions.insert("sinf");
	whitelistedFunctions.insert("tan");
	whitelistedFunctions.insert("tanf");
	whitelistedFunctions.insert("acos");
	whitelistedFunctions.insert("asin");
	whitelistedFunctions.insert("atan");
	whitelistedFunctions.insert("atan2");
	whitelistedFunctions.insert("cosh");
	whitelistedFunctions.insert("sinh");
	whitelistedFunctions.insert("tanh");
	whitelistedFunctions.insert("acosh");
	whitelistedFunctions.insert("asinh");
	whitelistedFunctions.insert("atanh");
	whitelistedFunctions.insert("exp");
	whitelistedFunctions.insert("log");
	whitelistedFunctions.insert("log10");
	whitelistedFunctions.insert("pow");
	whitelistedFunctions.insert("sqrt");
	whitelistedFunctions.insert("fmod");
	whitelistedFunctions.insert("fabs");
	whitelistedFunctions.insert("ceilf");
	whitelistedFunctions.insert("sqrtf");
}
