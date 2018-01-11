/*
 * mm_call_builder.h
 *
 *  Created on: Apr 22, 2013
 *      Author: rlyerly
 */

#ifndef MM_CALL_BUILDER_H_
#define MM_CALL_BUILDER_H_

namespace MMCallBuilder {
	SgExprStatement* buildMMInit(SgScopeStatement*);
	SgExprStatement* buildRegisterWrappersCall(SgScopeStatement*);
	SgExprStatement* buildUnregisterWrappersCall(SgScopeStatement*);
	SgExprStatement* buildRegisterPointerCall(SgExpression*, SgExpression*, SgScopeStatement*);
	SgExprStatement* buildUnregisterPointerCall(SgExpression*, SgScopeStatement*);
	SgExprStatement* buildGetSizeCall(SgExpression*, SgScopeStatement*);
	SgFunctionCallExp* buildGetSizeCallExp(SgExpression*, SgScopeStatement*);
	SgExprStatement* buildGetPointerCall(SgExpression*, SgScopeStatement*);
	SgFunctionCallExp* buildGetPointerCallExp(SgExpression*, SgScopeStatement*);
}

#endif /* MM_CALL_BUILDER_H_ */
