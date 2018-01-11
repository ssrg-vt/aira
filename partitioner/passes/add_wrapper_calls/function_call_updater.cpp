/*
 * function_call_updater.cpp
 *
 *  Created on: Apr 22, 2013
 *      Author: rlyerly
 */

#include "rose.h"
#include <sstream>

#include "common.h"
#include "add_wrapper_calls_common.h"
#include "function_call_updater.h"
#include "mm_call_builder.h"

/*
 * Default constructor
 */
FunctionCallUpdater::FunctionCallUpdater(SgFunctionDeclaration* p_function,
		Rose_STL_Container<SgNode*>& funcCalls) :
	function(p_function)
{
	ROSE_ASSERT(function);
	string msg = "Updating call sites for function \"" + NAME(function) + "\"";
	DEBUG(TOOL, msg);

	//Find call sites
	Rose_STL_Container<SgNode*>::const_iterator funcIt;
	SgFunctionCallExp* funcCall;
	for(funcIt = funcCalls.begin(); funcIt != funcCalls.end(); funcIt++)
	{
		funcCall = isSgFunctionCallExp(*funcIt);
		ROSE_ASSERT(funcCall);
		if(isSameFunction(funcCall->getAssociatedFunctionDeclaration(), function))
			callSites.insert(funcCall);
	}
}

/*
 * Update the function declaration's argument list so that it has size
 * arguments where necessary.
 */
void FunctionCallUpdater::updateDeclaration()
{
	Sg_File_Info* fileInfo;
	stringstream ss;
	fileInfo = function->get_file_info();
	ss << fileInfo->get_line();
	string msg = "Updating function declaration on line " + ss.str() + ", in file "
			+ fileInfo->get_filenameString();
	DEBUG(TOOL, msg);

	SgInitializedNamePtrList& args = function->get_args();
	SgInitializedNamePtrList::iterator argIt = args.begin();
	SgType* type = NULL;
	SgInitializedName* size = NULL;
	for(argIt = args.begin(); argIt != args.end(); argIt++)
	{
		type = (*argIt)->get_type();
		if(isSgArrayType(type) || isSgPointerType(type))
		{
			size = buildInitializedName(NAME(*argIt) + "__size", buildUnsignedLongType());
			argIt = args.insert(argIt + 1, size);
		}
	}
}

/*
 * Update call sites with size arguments for arrays/pointers
 */
void FunctionCallUpdater::updateSites()
{
	set<SgFunctionCallExp*>::const_iterator callIt;
	SgFunctionCallExp* call;
	Sg_File_Info* fileInfo;
	stringstream ss;
	SgType* type;
	string msg;

	if(callSites.size() == 0)
	{
		string msg = "No call sites for " + NAME(function);
		WARNING(TOOL, msg);
		return;
	}

	//Update sites
	for(callIt = callSites.begin(); callIt != callSites.end(); callIt++)
	{
		call = isSgFunctionCallExp(*callIt);
		ROSE_ASSERT(call);

		fileInfo = call->get_file_info();
		ss.str("");
		ss << fileInfo->get_line();
		msg = "Updating call site on line " + ss.str() + ", in file " + fileInfo->get_filenameString();
		DEBUG(TOOL, msg);

		//Iterate through the list to find pointer/array arguments
		//TODO sanitize argument list - right now we're assuming variable references
		SgExpressionPtrList& exprs = call->get_args()->get_expressions();
		SgExpressionPtrList::iterator exprIt;
		SgExpression* expr = NULL;
		for(exprIt = exprs.begin(); exprIt != exprs.end(); exprIt++)
		{
			expr = isSgExpression(*exprIt);
			ROSE_ASSERT(expr);

			type = expr->get_type();
			if(isSgArrayType(type) || isSgPointerType(type))
			{
				//Add size parameter
				SgFunctionCallExp* callExp = MMCallBuilder::buildGetSizeCallExp(expr, getScope(expr));
				exprIt = exprs.insert(exprIt + 1, callExp);
			}
		}
	}
}

/*
 * Static method to insert a call to initialize the wrappers in main
 */
void FunctionCallUpdater::insertInitWrapperCall(SgFunctionDeclaration* main)
{
	SgExprStatement* initCall = MMCallBuilder::buildMMInit(main->get_definition());
	main->get_definition()->prepend_statement(initCall);
}
