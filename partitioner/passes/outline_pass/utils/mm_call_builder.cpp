/*
 * mm_call_builder.cpp
 *
 *  Created on: Apr 22, 2013
 *      Author: rlyerly
 */

#include "rose.h"
#include "mm_call_builder.h"

using namespace SageBuilder;

namespace MMCallBuilder
{
	/*
	 * Build a call to the macro "INIT_MM_WRAPPER()"
	 */
	SgExprStatement* buildMMInit(SgScopeStatement* scope)
	{
		SgName name("INIT_MM_WRAPPER");
		return buildExprStatement(buildFunctionCallExp(name, buildVoidType(), NULL, scope));
	}

	/*
	 * Build a call that registers (enables) the MM wrapper
	 */
	SgExprStatement* buildRegisterWrappersCall(SgScopeStatement* scope)
	{
		SgName name("register_mm_wrappers");
		return buildExprStatement(buildFunctionCallExp(name, buildVoidType(), NULL, scope));
	}

	/*
	 * Build a call that unregisters (disables) the MM wrapper
	 */
	SgExprStatement* buildUnregisterWrappersCall(SgScopeStatement* scope)
	{
		SgName name("unregister_mm_wrappers");
		return buildExprStatement(buildFunctionCallExp(name, buildVoidType(), NULL, scope));
	}

	/*
	 * Build a call to register the specified pointer variable and a variable
	 * containing its size
	 */
	SgExprStatement* buildRegisterPointerCall(SgExpression* var, SgExpression* size,
			SgScopeStatement* scope)
	{
		SgType* type = var->get_type();
		ROSE_ASSERT(isSgArrayType(type) || isSgPointerType(type));
		type = size->get_type();
		ROSE_ASSERT(isSgTypeUnsignedInt(type));

		SgName name("register_pointer");
		SgExprListExp* arguments = buildExprListExp(var, size);
		return buildExprStatement(buildFunctionCallExp(name, buildVoidType(), arguments, scope));
	}

	/*
	 * Build a call to unregister the specified pointer variable
	 */
	SgExprStatement* buildUnregisterPointerCall(SgExpression* var, SgScopeStatement* scope)
	{
		SgType* type = var->get_type();
		ROSE_ASSERT(isSgArrayType(type) || isSgPointerType(type));

		SgName name("unregister_pointer");
		SgExprListExp* arguments = buildExprListExp(var);
		return buildExprStatement(buildFunctionCallExp(name, buildVoidType(), arguments, scope));
	}

	/*
	 * Build a call that returns the size of the specified pointer
	 * NOTE: This returns a statement
	 */
	SgExprStatement* buildGetSizeCall(SgExpression* var, SgScopeStatement* scope)
	{
		return buildExprStatement(buildGetSizeCallExp(var, scope));
	}

	/*
	 * Build a call that returns the size of the specified pointer
	 * NOTE: This returns an expression
	 */
	SgFunctionCallExp* buildGetSizeCallExp(SgExpression* var, SgScopeStatement* scope)
	{
		SgType* type = var->get_type();
		ROSE_ASSERT(isSgArrayType(type) || isSgPointerType(type));

		SgName name("get_size");
		SgExprListExp* arguments = buildExprListExp(var);
		return buildFunctionCallExp(name, buildUnsignedIntType(), arguments, scope);
	}

	/*
	 * Build a call that returns a pointer that points to the beginning of the
	 * block pointed to by the specified pointer variable (i.e. if the pointer
	 * variable points to an element in the middle of an array, this returns
	 * the address of the start of the array)
	 * NOTE: This returns a statement
	 */
	SgExprStatement* buildGetPointerCall(SgExpression* var, SgScopeStatement* scope)
	{
		return buildExprStatement(buildGetPointerCallExp(var, scope));
	}

	/*
	 * Build a call that returns a pointer that points to the beginning of the
	 * block pointed to by the specified pointer variable (i.e. if the pointer
	 * variable points to an element in the middle of an array, this returns
	 * the address of the start of the array)
	 * NOTE: This returns an expression
	 */
	SgFunctionCallExp* buildGetPointerCallExp(SgExpression* var, SgScopeStatement* scope)
	{
		SgType* type = var->get_type();
		ROSE_ASSERT(isSgArrayType(type) || isSgPointerType(type));

		SgName name("get_pointer");
		SgExprListExp* arguments = buildExprListExp(var);
		return buildFunctionCallExp(name, buildPointerType(), arguments, scope);
	}
}
