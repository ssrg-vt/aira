/*****************************************************************************/
/* Implementation of MathFeature class                                       */
/*****************************************************************************/

#include <iostream>
#include <string>
#include <llvm/IR/Function.h>

#include "MathFeature.h"

using namespace std;

MathFeature::MathFeature() :
		adds(0),
		subtracts(0),
		multiplies(0),
		divides(0),
		remainders(0),
		vec_adds(0),
		vec_subtracts(0),
		vec_multiplies(0),
		vec_divides(0),
		vec_remainders(0) {}

void MathFeature::clear()
{
	adds = 0;
	subtracts = 0;
	multiplies = 0;
	divides = 0;
	remainders = 0;
	vec_adds = 0;
	vec_subtracts = 0;
	vec_multiplies = 0;
	vec_divides = 0;
	vec_remainders = 0;
}

