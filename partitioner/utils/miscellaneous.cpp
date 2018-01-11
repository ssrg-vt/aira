/*
 * utils.c
 *
 *  Created on: Jul 22, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "miscellaneous.h"

namespace Misc
{

/*
 * Internal method
 *
 * Digs through typedef/arrays/pointers to get the base type & number of
 * dimensions.  The basetype & number of dimensions parameters are set by
 * this function.
 *
 * TODO this is pretty cloogy - is there a better way?
 */
void getType(SgType** origType, SgType** baseType, int* numDimensions)
{
	ROSE_ASSERT(origType != NULL);
	ROSE_ASSERT(*origType != NULL);
	ROSE_ASSERT(baseType != NULL);
	ROSE_ASSERT(numDimensions != NULL);

	//Dig through typedef/pointer/array types to get base type
	*origType = (*origType)->stripType(SgType::STRIP_TYPEDEF_TYPE);
	*numDimensions = 0;
	*baseType = NULL;
	if(isSgArrayType(*origType) || isSgPointerType(*origType))
	{
		(*numDimensions)++;
		*baseType = isSgArrayType(*origType) ? isSgArrayType(*origType)->get_base_type() :
				isSgPointerType(*origType)->get_base_type();
		*baseType = (*baseType)->stripType(SgType::STRIP_TYPEDEF_TYPE);
		while(isSgArrayType(*baseType) || isSgPointerType(*baseType))
		{
			(*numDimensions)++;
			*baseType = isSgArrayType(*baseType) ? isSgArrayType(*baseType)->get_base_type() :
				isSgPointerType(*baseType)->get_base_type();
			*baseType = (*baseType)->stripType(SgType::STRIP_TYPEDEF_TYPE);
		}
	}
}

/*
 * See if the defined class contains pointers, as this affects how the class is
 * transferred via MPI.
 *
 * TODO this check is performed when finding compatible architectures - can we
 * save that information rather than performing this check again?
 */
bool classContainsPointers(SgClassType* type)
{
	SgClassDeclaration* classDecl =
			isSgClassDeclaration(type->get_declaration()->get_definingDeclaration());
	ROSE_ASSERT(classDecl);
	SgClassDefinition* classDef = classDecl->get_definition();

	//Iterate through members, looking for pointers
	SgDeclarationStatementPtrList& members = classDef->get_members();
	SgDeclarationStatementPtrList::const_iterator memberIt = members.begin();
	SgVariableDeclaration* varDecl = NULL;
	for(memberIt = members.begin(); memberIt != members.end(); memberIt++)
	{
		varDecl = isSgVariableDeclaration(*memberIt);
		if(varDecl) //TODO ROSE doesn't seem to differentiate between data + methods as members
		{
			if(isSgPointerType(varDecl->get_definition()->get_type()))
				return true;
		}
	}
	return false;
}

}
