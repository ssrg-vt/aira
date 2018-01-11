/*****************************************************************************/
/* IntFeature class                                                          */
/*                                                                           */
/* This class collects integer math features (both signed & unsigned)        */
/*****************************************************************************/

#ifndef _INT_FEATURE_H
#define _INT_FEATURE_H

#include "MathFeature.h"

class IntFeature : public MathFeature {
public:
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);
};

#endif /* _INT_FEATURE_H */
