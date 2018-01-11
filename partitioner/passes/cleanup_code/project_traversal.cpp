/*
 * project_traversal.cpp - perform any necessary code cleanup during a tree
 * traversal.
 *
 *  Created on: Jul 16, 2013
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#include "rose.h"

#include "project_traversal.h"
#include "common.h"
#include "cleanup_code_common.h"

/*
 * Default constructor.
 */
ProjectTraversal::ProjectTraversal()
{
	// no-op for now
}

/*
 * Search the AST for code that needs to be cleaned up.
 */
void ProjectTraversal::visit(SgNode* node)
{
	SgVariableDeclaration* varDecl = isSgVariableDeclaration(node);
	SgExpression* expr = isSgExpression(node);

	if(varDecl)
		removeTypedef(varDecl);
	else if(expr)
		cleanupExpression(expr);
}

/*
 * Replace variables that are declared with a typedef type with their canonical
 * type.
 */
void ProjectTraversal::removeTypedef(SgVariableDeclaration* varDecl)
{
	string msg;

	SgInitializedNamePtrList& vars = varDecl->get_variables();
	SgInitializedNamePtrList::const_iterator varIt = vars.begin();
	SgInitializedName* var = NULL;
	SgType* type = NULL;
	SgArrayType* arrType = NULL;
	SgPointerType* ptrType = NULL;
	SgType* baseType = NULL;
	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		var = *varIt;

		/*string msg = "Removing typedefs for variable " + var->get_name();
		DEBUG(TOOL, msg);*/

		var->set_type(var->get_type()->stripType(SgType::STRIP_TYPEDEF_TYPE));
		type = var->get_type();
		while(isSgPointerType(type) || isSgArrayType(type))
		{
			arrType = isSgArrayType(type);
			ptrType = isSgPointerType(type);

			if(arrType)
			{
				baseType = arrType->get_base_type()->stripType(SgType::STRIP_TYPEDEF_TYPE);
				arrType->set_base_type(baseType);
			}
			else
			{
				baseType = ptrType->get_base_type()->stripType(SgType::STRIP_TYPEDEF_TYPE);
				ptrType->set_base_type(baseType);
			}

			type = baseType;
		}
	}
}

/*
 * Clean up the expression.
 */
void ProjectTraversal::cleanupExpression(SgExpression* expr)
{
	SgSizeOfOp* sizeOf = isSgSizeOfOp(expr);

	//TODO cleanup typedefs of types found in expressions
	if(sizeOf)
	{
		//Inspect the op
	}
}
