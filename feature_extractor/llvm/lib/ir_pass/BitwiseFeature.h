/*****************************************************************************/
/* BitwiseFeature class                                                      */
/*                                                                           */
/* This class collects features related to bitwise instructions              */
/*****************************************************************************/

#ifndef _BITWISE_FEATURE_H
#define _BITWISE_FEATURE_H

#include "IRFeature.h"

class BitwiseFeature : public IRFeature {
public:
	BitwiseFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

private:
	unsigned long lshifts;
	unsigned long rshifts;
	unsigned long ands;
	unsigned long ors;
	unsigned long xors;

	unsigned long vec_lshifts;
	unsigned long vec_rshifts;
	unsigned long vec_ands;
	unsigned long vec_ors;
	unsigned long vec_xors;
};

#endif /* _BITWISE_FEATURE_H */
