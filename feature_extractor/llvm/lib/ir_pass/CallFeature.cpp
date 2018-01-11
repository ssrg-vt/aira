/*****************************************************************************/
/* Implementation of CallFeature class                                    */
/*****************************************************************************/

#include <iostream>
#include <llvm/IR/Function.h>

#include "CallFeature.h"

using namespace std;
using namespace llvm;

CallFeature::CallFeature() :
		calls(0) {}

void CallFeature::clear()
{
	calls = 0;
}

void CallFeature::collect(Instruction& instr, double bb_count)
{
	if(instr.getOpcode() == Instruction::Call)
		calls += (unsigned long)bb_count;
}

void CallFeature::print(ostream& stream)
{
	stream << "Function calls: " << calls << endl;
}
