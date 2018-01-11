/*****************************************************************************/
/* CallFeature class                                                         */
/*                                                                           */
/* This class counts the number of function calls in the function.           */
/*****************************************************************************/

#ifndef _CALL_FEATURE_H
#define _CALL_FEATURE_H

#include "IRFeature.h"

class CallFeature : public IRFeature {
public:
	CallFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

private:
	unsigned long calls;
};

#endif /* _CALL_FEATURE_H */
