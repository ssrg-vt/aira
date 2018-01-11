/*****************************************************************************/
/* Implementation of IRFeature class                                         */
/*****************************************************************************/

#include <string>
#include <llvm/IR/Function.h>

#include "IRFeature.h"

/*
 * Return true if the specified value is a vector type, or false otherwise.
 */
bool IRFeature::isVectorType(llvm::Value* val)
{
	return val->getType()->isVectorTy();
}

