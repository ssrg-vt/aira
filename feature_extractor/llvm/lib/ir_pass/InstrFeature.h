/*****************************************************************************/
/* InstrFeature class                                                        */
/*                                                                           */
/* This class simply counts the number of instructions in a function         */
/*****************************************************************************/

#ifndef _INSTR_FEATURE_H
#define _INSTR_FEATURE_H

#include "IRFeature.h"

class InstrFeature : public IRFeature {
public:
	InstrFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

private:
	unsigned long instructions;
};

#endif /* _INSTR_FEATURE_H */
