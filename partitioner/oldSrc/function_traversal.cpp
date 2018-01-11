/*
 * function_traversal.cpp
 *
 *  Created on: Jan 22, 2013
 *      Author: rlyerly
 */

#include "rose.h" // Must come first for PCH to work.
#include <iostream>
#include "function_declaration.h"
#include "function_traversal.h"

using namespace std;
using namespace SageInterface;
using namespace NodeQuery;

FunctionTraversal::FunctionTraversal(FunctionDeclaration* function) :
		function(function),
		undefinedFunctionsCalled(false)
{}

/*
 * Analyze this node (in the kernel subtree) for specific characteristics, including:
 * 	- global variables read/written
 * 	- function side-effects
 * 	- functions called (that need to be moved to the partition)
 * 	- undefined functions (not allowed)
 * 	- TODO user-defined datatypes needed by this function
 */
void FunctionTraversal::preOrderVisit(SgNode* node)
{
	SgVarRefExp* varRef = isSgVarRefExp(node);
	SgFunctionCallExp* funcCall = isSgFunctionCallExp(node);
	SgAssignOp* assignOp;

	//Don't need to check for side-effects if a pure function
	if(function->functionDeclaration->get_functionModifier().isPure())
		assignOp = NULL;
	else
		assignOp = isSgAssignOp(node);

	SgInitializedName* declaration;
	SgVariableSymbol* symbol;

	if(varRef)	//Check for global values read/written
	{
		symbol = varRef->get_symbol();
		declaration = symbol->get_declaration();
		bool isWritten = varRef->get_lvalue();

		if(isSgGlobal(declaration->get_scope()))
		{
			if(!function->contains(function->globalInputs, declaration))
				function->globalInputs.push_back(declaration);

			if(isWritten && !function->contains(function->globalOutputs, declaration))
				function->globalOutputs.push_back(declaration);
		}
	}
	else if(assignOp)	//Check for side-effects
	{
		SgPointerDerefExp* pointerDeref = isSgPointerDerefExp(assignOp->get_lhs_operand());
		SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(assignOp->get_lhs_operand());
		if(pointerDeref || arrayRef)
		{
			if(pointerDeref)	//Pointer dereference
				varRef = isSgVarRefExp(pointerDeref->get_operand());
			else	//Array index
				varRef = isSgVarRefExp(arrayRef->get_lhs_operand());

			if(varRef)
			{
				symbol = varRef->get_symbol();
				declaration = symbol->get_declaration();

				if(function->contains(*function->inputs, declaration) &&
						!function->contains(function->outputs, declaration))
				{
					function->outputs.push_back(declaration);
					//TODO find a better way to add the size to outputs
					for(SgInitializedNamePtrList::const_iterator it = (*function->inputs).begin();
						it != (*function->inputs).end(); it++)
					{
						if(*it == declaration)
							function->outputs.push_back(*(it + 1));
					}
				}
			}
		}
	}
	else if(funcCall)	//Retain functions called by kernel/check for undefined functions and side-effects
	{
		SgFunctionDeclaration* funcDecl =
				isSgFunctionDeclaration(
						funcCall->getAssociatedFunctionDeclaration()->get_definingDeclaration());

		if(!funcDecl)
		{
			//TODO implement notion of white-listed functions
			undefinedFunctionsCalled = true;
			cout << "Undefined function \"" <<
					funcCall->getAssociatedFunctionSymbol()->get_name().getString() << "\" found" << endl;
		}
		else
		{
			if(!function->contains(function->functionsCalled, funcDecl))
				function->functionsCalled.push_back(funcDecl);

			//Right now, if a pointer/array input to the kernel entry point is passed as a parameter
			//to a sub-call, we assume that parameter will have side affects.  Check the parameters
			//of this function call to see if they correspond to the kernel inputs.  If so, add them
			//to the outputs list - this is a TODO to change to deeper analysis
			if(getScope(funcCall) == function->functionBody)
			{
				SgExpressionPtrList funcArgs = funcCall->get_args()->get_expressions();
				SgExpressionPtrList::const_iterator it;
				for(it = funcArgs.begin(); it != funcArgs.end(); it++)
				{
					Rose_STL_Container<SgNode*> varRefs = querySubTree(*it, V_SgVarRefExp);
					for(Rose_STL_Container<SgNode*>::const_iterator varIt = varRefs.begin();
								varIt != varRefs.end();
								varIt++)
					{
						varRef = isSgVarRefExp(*varIt);
						ROSE_ASSERT(varRef != NULL);

						if(!isSgArrayType(varRef->get_type()) && !isSgPointerType(varRef->get_type()))
							continue;

						symbol = varRef->get_symbol();
						declaration = symbol->get_declaration();

						if(function->contains(*function->inputs, declaration) &&
								!function->contains(function->outputs, declaration))
						{
							function->outputs.push_back(declaration);
							//TODO find a better way to store the size
							for(SgInitializedNamePtrList::const_iterator it = (*function->inputs).begin();
								it != (*function->inputs).end(); it++)
							{
								if(*it == declaration)
									function->outputs.push_back(*(it + 1));
							}
						}
					}
				}
			}
		}
	}

	return;
}

/*
 * Perform any post-processing necessary when leaving this sub-tree
 */
void FunctionTraversal::postOrderVisit(SgNode* node)
{
	//no-op for now...
}

/*
 * Return whether or not the function calls any undefined functions, as found
 * by the tree traversal.
 */
bool FunctionTraversal::callsUndefinedFunctions()
{
	return undefinedFunctionsCalled;
}
