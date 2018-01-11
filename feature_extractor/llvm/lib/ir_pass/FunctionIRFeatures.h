/*****************************************************************************/
/* FunctionFeatures class                                                    */
/*                                                                           */
/* This class encapsulates all the IRFeatures collected by a given function. */
/*****************************************************************************/

#ifndef _FUNCTION_IR_FEATURES_H
#define _FUNCTION_IR_FEATURES_H

#include <vector>
#include <iostream>

#include "IRFeature.h"

class FunctionIRFeatures {
public:
	FunctionIRFeatures(llvm::Function& F);
	~FunctionIRFeatures();

	void clear();
	void extract(llvm::Instruction& instr, double bb_count);
	void print(std::ostream& stream);

private:
	std::vector<IRFeature*> features;
};

#endif /* _FUNCTION_IR_FEATURES_H */
