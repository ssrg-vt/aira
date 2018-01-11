/*
 * function_interface.cpp
 *
 *  Created on: Apr 9, 2013
 *      Author: rlyerly
 */

/* ROSE headers */
#include "rose.h"

/* Function interface headers */
#include "common.h"
#include "kernel_interface_common.h"
#include "function_interface.h"
#include "function_traversal.h"
#include "pragma_handling.h"

/*
 * Initialized the function interface object.
 */
FunctionInterface::FunctionInterface(SgFunctionDeclaration* p_function, SgGraphNode* p_node) :
	function(p_function),
	node(p_node),
	status(NOT_ANALYZED),
	shouldAnnotate(false)
{
	//See if this function contains OpenMP pragmas - if so, annotate
	Rose_STL_Container<SgNode*> pragmas = querySubTree(p_function, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt = pragmas.begin();
	SgPragmaDeclaration* pragma;
	for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
	{
		pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma != NULL);

		if(extractPragmaKeyword(pragma) == "omp")
		{
			shouldAnnotate = true;
			break;
		}
	}
}

/*
 * Save the functions that this function calls.
 */
void FunctionInterface::storeFunctionsCalled(set<FunctionInterface*> funcsCalled)
{
	calledFunctions = funcsCalled;
}

/*
 * Discover this function's kernel interface, including I/O and functions
 * needed.
 */
void FunctionInterface::analyze()
{
	string msg = "Analyzing function " + NAME(function);
	DEBUG(TOOL, msg);

	status = IN_PROGRESS;

	//Add inputs to our list
	SgInitializedNamePtrList inputs = function->get_parameterList()->get_args();
	SgInitializedNamePtrList::const_iterator inputIt;
	for(inputIt = inputs.begin(); inputIt != inputs.end(); inputIt++)
	{
		msg = "\tfound input " + NAME(*inputIt);
		DEBUG(TOOL, msg);
		addInput(*inputIt);

		FunctionTraversal::checkVariableType((*inputIt)->get_type(), this);
	}

	//Add return value to our list (denoted by special marker name "__retval__")
	SgType* returnType = function->get_orig_return_type();
	if(!isSgTypeVoid(returnType))
	{
		msg = "\tfound output of type " + get_name(returnType);
		DEBUG(TOOL, msg);
		SgInitializedName* returnVal = buildInitializedName(RETVAL_NAME, returnType);
		addOutput(returnVal);

		FunctionTraversal::checkVariableType(returnType, this);
	}

	//Traverse function
	FunctionTraversal ft(this);
	ft.traverse(function, preorder);

	//Incorporate sub-calls
	//TODO do side-effect checking here for sub-calls?
	set<FunctionInterface*>::const_iterator funcIt;
	for(funcIt = calledFunctions.begin(); funcIt != calledFunctions.end(); funcIt++)
	{
		if((*funcIt)->getStatus() == NOT_ANALYZED)
			(*funcIt)->analyze();

		combineGlobalInputs((*funcIt)->getGlobalInputs());
		combineGlobalOutputs((*funcIt)->getGlobalOutputs());
		combineCalledFunctions((*funcIt)->getCalledFunctions());
	}

	status = ANALYZED;
}

/*
 * Annotate this function with the kernel's interface.
 */
void FunctionInterface::annotate()
{
	if(!shouldAnnotate)
		return;

	string msg = "Annotating function " + NAME(function);
	DEBUG(TOOL, msg);

	//Insert the pragmas
	vector<SgStatement*> pragmas(6);
	SgScopeStatement* scope = function->get_scope();
	pragmas[0] = PragmaBuilder::buildVariablePragma(INPUT, inputs, scope);
	pragmas[1] = PragmaBuilder::buildVariablePragma(GLOBAL_INPUT, globalInputs, scope);
	pragmas[2] = PragmaBuilder::buildVariablePragma(OUTPUT, outputs, scope);
	pragmas[3] = PragmaBuilder::buildVariablePragma(GLOBAL_OUTPUT, globalOutputs, scope);
	set<SgFunctionDeclaration*> functions;
	set<FunctionInterface*>::const_iterator funcIt;
	for(funcIt = calledFunctions.begin(); funcIt != calledFunctions.end(); funcIt++)
		functions.insert((*funcIt)->getDeclaration());
	pragmas[4] = PragmaBuilder::buildFunctionPragma(functions, scope);
	pragmas[5] = PragmaBuilder::buildClassesPragma(classesNeeded, scope);

	insertStatementList(function, pragmas);
}

/*
 * Get this function's analysis status.
 */
enum analysisStatus FunctionInterface::getStatus()
{
	return status;
}

/*
 * Get the call graph node corresponding to this function.
 */
SgGraphNode* FunctionInterface::getGraphNode()
{
	return node;
}

/*
 * Get this function's declaration.
 */
SgFunctionDeclaration* FunctionInterface::getDeclaration()
{
	return function;
}

/*
 * Get this function's global inputs.
 */
set<SgInitializedName*> FunctionInterface::getGlobalInputs()
{
	return globalInputs;
}

/*
 * Get this function's global outputs.
 */
set<SgInitializedName*> FunctionInterface::getGlobalOutputs()
{
	return globalOutputs;
}

/*
 * Get the functions this function calls.
 */
set<FunctionInterface*> FunctionInterface::getCalledFunctions()
{
	return calledFunctions;
}

/*
 * Store an input for this kernel.
 */
bool FunctionInterface::addInput(SgInitializedName* variable)
{
	return inputs.insert(variable).second;
	/*if(inputs.find(variable) == inputs.end())
	{
		inputs.insert(variable);
		return true;
	}
	return false;*/
}

/*
 * Store an output for this kernel.
 */
bool FunctionInterface::addOutput(SgInitializedName* variable)
{
	return outputs.insert(variable).second;
	/*if(outputs.find(variable) == outputs.end())
	{
		outputs.insert(variable);
		return true;
	}
	return false;*/
}

/*
 * Store a global input for this kernel.
 */
bool FunctionInterface::addGlobalInput(SgInitializedName* variable)
{
	return globalInputs.insert(variable).second;
	/*if(globalInputs.find(variable) == globalInputs.end())
	{
		globalInputs.insert(variable);
		return true;
	}
	return false;*/
}

/*
 * Store a global output for this kernel.
 */
bool FunctionInterface::addGlobalOutput(SgInitializedName* variable)
{
	return globalOutputs.insert(variable).second;
	/*if(globalOutputs.find(variable) == globalOutputs.end())
	{
		globalOutputs.insert(variable);
		return true;
	}
	return false;*/
}

/*
 * Save a class needed by this kernel.
 */
bool FunctionInterface::addClassNeeded(SgClassType* ctype)
{
	return classesNeeded.insert(ctype).second;
}

/*
 * Combine this function's global inputs with another function's.
 */
void FunctionInterface::combineGlobalInputs(set<SgInitializedName*> p_globalInputs)
{
	set<SgInitializedName*>::const_iterator varIt;
	for(varIt = p_globalInputs.begin(); varIt != p_globalInputs.end(); varIt++)
		globalInputs.insert(*varIt);
		/*if(globalInputs.find(*varIt) == globalInputs.end())
			globalInputs.insert(*varIt);*/
}

/*
 * Combine this function's global outputs with another function's.
 */
void FunctionInterface::combineGlobalOutputs(set<SgInitializedName*> p_globalOutputs)
{
	set<SgInitializedName*>::const_iterator varIt;
	for(varIt = p_globalOutputs.begin(); varIt != p_globalOutputs.end(); varIt++)
		globalOutputs.insert(*varIt);
		/*if(globalOutputs.find(*varIt) == globalOutputs.end())
			globalOutputs.insert(*varIt);*/
}

/*
 * Combine this function's called functions with another function's.
 */
void FunctionInterface::combineCalledFunctions(set<FunctionInterface*> p_calledFunctions)
{
	set<FunctionInterface*>::const_iterator funcIt;
	for(funcIt = p_calledFunctions.begin(); funcIt != p_calledFunctions.end(); funcIt++)
		calledFunctions.insert(*funcIt);
		/*if(calledFunctions.find(*funcIt) == calledFunctions.end())
			calledFunctions.insert(*funcIt);*/
}
