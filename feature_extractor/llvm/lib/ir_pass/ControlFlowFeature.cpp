/*****************************************************************************/
/* Implementation of ControlFlowFeature class                                */
/*****************************************************************************/

#include <iostream>
#include <string>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

#include "ControlFlowFeature.h"

using namespace std;
using namespace llvm;

ControlFlowFeature::ControlFlowFeature() :
	cond_branches(0),
	uncond_branches(0),
	switches(0) {}

void ControlFlowFeature::clear()
{
	cond_branches = 0;
	uncond_branches = 0;
	switches = 0;
}

void ControlFlowFeature::collect(Instruction& instr, double bb_count)
{
	//Because of the perfect storm of references, switch statements and cast
	//operations, break the check for branch instructions away from the rest
	if(isa<BranchInst>(instr))
	{
		BranchInst& br_instr = cast<BranchInst>(instr);
		if(br_instr.isConditional())
			cond_branches += (unsigned long)bb_count;
		else
			uncond_branches += (unsigned long)bb_count;
	}
	else
	{
		unsigned int opcode = instr.getOpcode();
		switch(opcode)
		{
		case Instruction::Switch:
			switches += (unsigned long)bb_count;
			break;
		case Instruction::IndirectBr:
			uncond_branches += (unsigned long)bb_count;
			break;
		default:
			//Nothing for now...
			break;
		}
	}
}

void ControlFlowFeature::print(ostream& stream)
{
	stream << "Conditional branches: " << cond_branches << endl;
	stream << "Unconditional branches: " << uncond_branches << endl;
	stream << "Switches: " << switches << endl;
}
