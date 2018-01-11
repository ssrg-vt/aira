/*
 * function.cpp - Implementation of the Function class defined in function.h
 *
 *  Created on: Jan 21, 2013
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

/* Must come first for PCH to work */
#include "rose.h"

#include <iostream>

#include "function_call.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

int FunctionCall::constExprNum = 0;

/**
 * Default constructor.  Sets up the object for use in partitioning.
 */
FunctionCall::FunctionCall(SgFunctionCallExp* functionCall) :
	functionCall(functionCall),
	partitionStatus(CALL_SITE_INITIALIZED)
{}

/*
 * Update the call site to reflect the partitioned nature of the function.
 */
bool FunctionCall::updateCallSite()
{
	partitionStatus = CALL_SITE_UPDATED;

	//Sanitize the argument list, i.e. lift every non-variable expression into
	//a variable
	if(!sanitizeArgumentList())
	{
		partitionStatus = COULD_NOT_SANITIZE;
		return false;
	}

	//Add size arguments for any arrays/pointers being passed
	if(!addSizeArguments())
	{
		partitionStatus = INVALID_ARGUMENT_LIST;
		return false;
	}

	return true;
}

/*
 * Lifts all non-variable reference expressions out of the argument list and
 * into temporary variables, which replace the expressions in the argument list.
 *
 * This simplifies analysis for adding size arguments later on.
 */
bool FunctionCall::sanitizeArgumentList()
{
	SgExpressionPtrList& callArgs = functionCall->get_args()->get_expressions();
	SgExpressionPtrList::iterator it;
	SgCastExp* castExp;
	SgVarRefExp* varRefExp;
	stringstream ss;
	string type, constName;

	//Check for non-variable reference expressions inside the argument list.  If any
	//are found, lift them outside of the function call and store them in a variable.
	for(it = callArgs.begin(); it != callArgs.end(); it++)
	{
		castExp = isSgCastExp(*it);
		varRefExp = isSgVarRefExp(*it);

		if(castExp)
		{
			//Get expression being cast
			while(isSgCastExp(castExp->get_operand()))
				castExp = isSgCastExp(castExp->get_operand());
			varRefExp = isSgVarRefExp(castExp->get_operand());
		}

		if(!varRefExp)
		{
			//Not a variable reference - declare placeholder & pull outside of argument list
			ss << constExprNum++;
			type = (*it)->get_type()->class_name().substr(2, type.length() - 2);
			constName = "__" + type + "_VarExpr" + ss.str();
			SgAssignInitializer* initializer = buildAssignInitializer(*it, (*it)->get_type());
			SgVariableDeclaration* varDecl = buildVariableDeclaration(constName,
					(*it)->get_type(), initializer, getScope(this->functionCall));
			insertStatement(getEnclosingStatement(functionCall), varDecl);
			(*it) = buildVarRefExp(varDecl); //TODO memory leak?  Should we use replaceExpression?
		}
	}

	return true;
}

/*
 * Add size arguments for arrays and pointers in the argument list.
 */
bool FunctionCall::addSizeArguments()
{
	SgExpressionPtrList& callArgs = functionCall->get_args()->get_expressions();
	SgExpressionPtrList::iterator it;
	SgCastExp* castExp;
	SgVarRefExp* varRefExp;
	SgUnsignedLongVal* arraySizeVal;

	//The list should be sanitized, i.e. it should only have variable references
	for(it = callArgs.begin(); it != callArgs.end(); it++)
	{
		castExp = isSgCastExp(*it);
		varRefExp = isSgVarRefExp(*it);

		if(castExp)
		{
			//Get expression being cast
			while(isSgCastExp(castExp->get_operand()))
				castExp = isSgCastExp(castExp->get_operand());
			varRefExp = isSgVarRefExp(castExp->get_operand());
		}
		ROSE_ASSERT(varRefExp != NULL);
		SgArrayType* arrayType = isSgArrayType(varRefExp->get_type());
		SgPointerType* pointerType = isSgPointerType(varRefExp->get_type());

		if(arrayType)
		{
			//Insert the array size into the argument list
			//TODO is there ever a time when array sizes aren't known at compile time (besides VLAs)?
			arraySizeVal = buildUnsignedLongVal(getArrayElementCount(arrayType));
			it = callArgs.insert(it + 1, arraySizeVal);
		}
		else if(pointerType)
		{
			//TODO calculate pointer size.  For now return false
			return false;
		}
	}

	return true;
}

bool FunctionCall::contains(SgInitializedNamePtrList& list, SgInitializedName* var)
{
	SgInitializedNamePtrList::const_iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		if(*it == var)
			return true;
	}
	return false;
}
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
