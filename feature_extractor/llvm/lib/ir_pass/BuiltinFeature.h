/*****************************************************************************/
/* BuiltinFeature class                                                      */
/*                                                                           */
/* This class counts the number of instrinsic function calls in the function.*/
/*****************************************************************************/

#ifndef _BUILTIN_FEATURE_H
#define _BUILTIN_FEATURE_H

#include "IRFeature.h"

class BuiltinFeature : public IRFeature {
public:
	BuiltinFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

private:
	unsigned long builtins;
	unsigned long vec_builtins;

	static const char* builtin_names[];

	bool findIn(std::string& first, std::string& second);
};

#endif /* _BUILTIN_FEATURE_H */
