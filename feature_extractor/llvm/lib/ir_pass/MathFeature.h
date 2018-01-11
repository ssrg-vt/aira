/******************************************************************************/
/* MathFeature class                                                          */
/*                                                                            */
/* This class is a template for collecting basic math features                */
/******************************************************************************/

#ifndef _MATH_FEATURE_H
#define _MATH_FEATURE_H

#include "IRFeature.h"

class MathFeature : public IRFeature {
public:
	MathFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count) = 0;
	virtual void print(std::ostream& stream) = 0;

protected:
	unsigned long adds;
	unsigned long subtracts;
	unsigned long multiplies;
	unsigned long divides;
	unsigned long remainders;

	unsigned long vec_adds;
	unsigned long vec_subtracts;
	unsigned long vec_multiplies;
	unsigned long vec_divides;
	unsigned long vec_remainders;
};

#endif /* _MATH_FEATURE_H */
