/*
 * function.h - Interface for a Function object, which encapsulates all the
 * logic necessary to extract a function from/insert a function into an AST.
 * This class also includes the ability to query the function's interface,
 * i.e. the inputs and outputs of the function.
 *
 *  Created on: Jan 21, 2013
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include "partition_status.h"

#ifndef FUNCTION_CALL_H_
#define FUNCTION_CALL_H_

class FunctionCall {
public:
	/*
	 * Default constructor.  Sets up the object and prepares the function for
	 * partitioning, if requested.
	 */
	FunctionCall(SgFunctionCallExp* functionCall);

	/*
	 * Updates the call site by lifting all non-variable reference expressions
	 * out of the parameter list and adds parameters for size arguments of
	 * arrays and pointers.
	 */
	bool updateCallSite();

private:
	/*
	 * General partitioning variables
	 */
	SgFunctionCallExp* functionCall;
	enum status partitionStatus;

	/*
	 * Sanitizes the argument list so that it only contains variable references.
	 */
	bool sanitizeArgumentList();

	/*
	 * Searches the input list for arrays and pointers and adds size arguments
	 * as needed.
	 */
	bool addSizeArguments();

	/*
	 * Checks the passed list to see if it contains the passed variable.
	 */
	bool contains(SgInitializedNamePtrList& list, SgInitializedName* var);

	/*
	 * Keeps count of the number of expressions lifted out of call sites, in addition
	 * to helping us avoid name collisions
	 */
	static int constExprNum;
};

#endif /* FUNCTION_CALL_H_ */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
