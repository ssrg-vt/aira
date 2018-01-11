/*****************************************************************************/
/* Implementation of InstrFeature class                                      */
/*****************************************************************************/

#include <iostream>
#include <string>
#include <llvm/IR/Function.h>

#include "InstrFeature.h"

using namespace std;
using namespace llvm;

InstrFeature::InstrFeature() :
		instructions(0) {}

void InstrFeature::clear()
{
	instructions = 0;
}

void InstrFeature::collect(Instruction& instr, double bb_count)
{
	instructions += (unsigned int)bb_count;
}

void InstrFeature::print(ostream& stream)
{
	stream << "Number of instructions: " << instructions << endl;
}
