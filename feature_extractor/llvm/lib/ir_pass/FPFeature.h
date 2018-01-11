/******************************************************************************/
/* FPFeature class                                                            */
/*                                                                            */
/* This class collects floating-point math features                           */
/******************************************************************************/

#ifndef _FP_FEATURE_H
#define _FP_FEATURE_H

#include "MathFeature.h"

class FPFeature : public MathFeature {
public:
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);
};

#endif /* _FP_FEATURE_H */
