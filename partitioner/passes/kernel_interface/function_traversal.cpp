/*
 * function_traversal.cpp
 *
 *  Created on: Apr 9, 2013
 *      Author: rlyerly
 */

/* ROSE headers */
#include "rose.h"

/* Function traversal headers */
#include "common.h"
#include "kernel_interface_common.h"
#include "function_interface.h"
#include "function_traversal.h"
#include "miscellaneous.h"

/*
 * Constructor for the traversal object.
 */
FunctionTraversal::FunctionTraversal(FunctionInterface* p_function) :
	function(p_function)
{
	ROSE_ASSERT(p_function);
	isPure = function->function->get_functionModifier().isPure();
	functionName = NAME(function->function);
}

/*
 * Visit each node in the sub-tree to discover the kernel interface.
 */
void FunctionTraversal::visit(SgNode* node)
{
	//Vars to determine what we need to inspect
	SgVarRefExp* varRef = isSgVarRefExp(node);
	SgAssignOp* assignOp = isSgAssignOp(node);
	SgCompoundAssignOp* opGetOp = isSgCompoundAssignOp(node);
	SgFunctionCallExp* funcCall = isSgFunctionCallExp(node);
	SgVariableDeclaration* varDecl = isSgVariableDeclaration(node);

	if(varRef) //Check for global variables read/written
		checkGlobalVars(varRef);
	else if(!isPure && (assignOp || opGetOp)) //Check for side-effects
	{
		if(assignOp)
			checkSideEffects(assignOp);
		else
			checkSideEffects(opGetOp);
	}
	else if(funcCall) //Check for side-effects of sub-calls
		checkFunctionCalls(funcCall);
	else if(varDecl)
		checkVariableType(varDecl->get_definition()->get_type());

}

/*
 * Check to see if this function reads/writes a global variable
 */
void FunctionTraversal::checkGlobalVars(SgVarRefExp* varRef)
{
	SgVariableSymbol* symbol = varRef->get_symbol();
	SgInitializedName* declaration = symbol->get_declaration();
	string msg;

	if(isSgGlobal(declaration->get_scope()))
	{
		//Check to see if it is a class type
		if(isSgVariableDefinition(declaration->get_definition()))
			checkVariableType(isSgVariableDefinition(declaration->get_definition())->get_type());

		//TODO distinguish between read/write.  Sometimes the variable is only
		//read, sometimes it is only written, sometimes it is both.  Right now
		//naively assume that if we find it, it is read

		if(function->addGlobalInput(declaration))
		{
			msg = "\tfound global variable " + NAME(symbol);
			DEBUG(TOOL, msg);
		}

		if(varRef->get_lvalue())
		{
			if(function->addGlobalOutput(declaration))
			{
				msg = "\tfound write to global variable " + NAME(symbol);
				DEBUG(TOOL, msg);
			}
		}
	}
}

/*
 * Check to see if this function has side-effects.
 */
void FunctionTraversal::checkSideEffects(SgBinaryOp* assignOp)
{
	SgExpression* op = assignOp->get_lhs_operand();
	SgDotExp* dotExp = isSgDotExp(op);
	SgPointerDerefExp* pointerDeref = NULL;
	SgPntrArrRefExp* arrayRef = NULL;
	SgVariableSymbol* symbol = NULL;
	SgInitializedName* declaration = NULL;
	SgVarRefExp* varRef = NULL;
	string msg;

	//Wade through dot expressions (member field accesses)
	if(dotExp)
	{
		op = dotExp->get_lhs_operand();
		while(isSgDotExp(op))
			op = isSgDotExp(op)->get_lhs_operand();
	}

	//Wade through derefs
	pointerDeref = isSgPointerDerefExp(op);
	arrayRef = isSgPntrArrRefExp(op);
	if(pointerDeref)
	{
		op = pointerDeref->get_operand();
		while(isSgPointerDerefExp(op))
			op = isSgPointerDerefExp(op)->get_operand();
		varRef = isSgVarRefExp(op);
	}
	else if(arrayRef)
	{
		//TODO dereferences, i.e. (*var)[k], doesn't work
		op = arrayRef->get_lhs_operand();
		while(isSgPntrArrRefExp(op))
			op = isSgPntrArrRefExp(op)->get_lhs_operand();
		varRef = isSgVarRefExp(op);
	}

	//If we're still a var
	//TODO if it is an array defined within this function, then it does NOT
	//have side effects.  If it is a pointer defined in this function, it
	//MAY have side effects, depending on where it points to...
	if(varRef)
	{
		symbol = varRef->get_symbol();
		declaration = symbol->get_declaration();

		if(function->inputs.find(declaration) != function->inputs.end())
		{
			if(function->addOutput(declaration))
			{
				msg = "\tfound side-effects for variable " + NAME(symbol);
				DEBUG(TOOL, msg);
			}
		}
	}
}

/*
 * Check for side-effects in sub-calls.
 *
 * TODO right now this is naive, in that if we call a function with a pointer
 * argument, we assume side-effects.  How do we do this with tighter analysis?
 */
void FunctionTraversal::checkFunctionCalls(SgFunctionCallExp* funcCall)
{
	SgVarRefExp* varRef = NULL;
	SgVariableSymbol* symbol = NULL;
	SgInitializedName* declaration = NULL;
	SgFunctionDeclaration* funcDecl =
			isSgFunctionDeclaration(funcCall->getAssociatedFunctionDeclaration()->
					get_definingDeclaration());
	SgExpressionPtrList funcArgs;
	SgExpressionPtrList::const_iterator argIt;
	Rose_STL_Container<SgNode*> varRefs;
	Rose_STL_Container<SgNode*>::const_iterator varIt;
	SgType* type;
	string msg;

	if(funcDecl)
	{
		msg = "\tfound call to function " + NAME(funcDecl);

		//Naive: if a function is called with a pointer/array input to the
		//kernel as an argument, we assume there will be side-effects
		funcArgs = funcCall->get_args()->get_expressions();
		for(argIt = funcArgs.begin(); argIt != funcArgs.end(); argIt++)
		{
			varRefs = querySubTree(*argIt, V_SgVarRefExp);
			for(varIt = varRefs.begin(); varIt != varRefs.end(); varIt++)
			{
				varRef = isSgVarRefExp(*varIt);
				ROSE_ASSERT(varRef);

				type = varRef->get_type();
				if(!isSgArrayType(type) && !isSgPointerType(type))
					continue;

				symbol = varRef->get_symbol();
				declaration = symbol->get_declaration();

				if(function->inputs.find(declaration) != function->inputs.end())
				{
					msg = "\tfound side-effects for variable " + NAME(symbol);
					DEBUG(TOOL, msg);
					function->addOutput(declaration);
				}
			}
		}
	}
}

/*
 * Check variable declarations for classes that need to be declared on the
 * partition side.
 */
void FunctionTraversal::checkVariableType(SgType* type)
{
	checkVariableType(type, function);
}

/*
 * Internal static method.  Allows checking from other contexts, besides the
 * traversal mechanism.
 */
void FunctionTraversal::checkVariableType(SgType* type, FunctionInterface* function)
{
	SgType* baseType = NULL;
	int numDimensions = 0;
	Misc::getType(&type, &baseType, &numDimensions);
	string msg;

	SgType* curType = (baseType != NULL ? baseType : type);
	SgClassType* classType = isSgClassType(curType);
	if(classType)
	{
		if(function->addClassNeeded(classType))
		{
			msg = "\tfound variable needing class declaration " + NAME(classType);
			DEBUG(TOOL, msg);
		}
	}
}
