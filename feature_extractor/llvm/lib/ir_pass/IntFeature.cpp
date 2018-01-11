/*****************************************************************************/
/* Implementation of IntFeature class                                        */
/*****************************************************************************/

#include <iostream>
#include <string>
#include <llvm/IR/Function.h>

#include "IntFeature.h"

using namespace std;
using namespace llvm;

void IntFeature::collect(Instruction& instr, double bb_count)
{
	if(instr.getNumOperands() < 1)
		return;

	unsigned int opcode = instr.getOpcode();
	if(isVectorType(instr.getOperand(0)))
	{
		switch(opcode)
		{
		case Instruction::Add:
			vec_adds += (unsigned int)bb_count;
			break;
		case Instruction::Sub:
			vec_subtracts += (unsigned int)bb_count;
			break;
		case Instruction::Mul:
			vec_multiplies += (unsigned int)bb_count;
			break;
		case Instruction::UDiv:
		case Instruction::SDiv:
			vec_divides += (unsigned int)bb_count;
			break;
		case Instruction::URem:
		case Instruction::SRem:
			vec_remainders += (unsigned int)bb_count;
			break;
		default:
			//Nothing for now...
			break;
		}
	}
	else
	{
		switch(opcode)
		{
		case Instruction::Add:
			adds += (unsigned int)bb_count;
			break;
		case Instruction::Sub:
			subtracts += (unsigned int)bb_count;
			break;
		case Instruction::Mul:
			multiplies += (unsigned int)bb_count;
			break;
		case Instruction::UDiv:
		case Instruction::SDiv:
			divides += (unsigned int)bb_count;
			break;
		case Instruction::URem:
		case Instruction::SRem:
			remainders += (unsigned int)bb_count;
			break;
		default:
			//Nothing for now...
			break;
		}
	}
}

void IntFeature::print(ostream& stream)
{
	stream << "Integer additions: " << adds << endl;
	stream << "Integer subtractions: " << subtracts << endl;
	stream << "Integer multiplications: " << multiplies << endl;
	stream << "Integer divisions (s+u): " << divides << endl;
	stream << "Integer remainders (s+u): " << remainders << endl;
	stream << "Integer vector additions: " << vec_adds<< endl;
	stream << "Integer vector subtractions: " << vec_subtracts<< endl;
	stream << "Integer vector multiplications: " << vec_multiplies << endl;
	stream << "Integer vector divisions (s+u): " << vec_divides << endl;
	stream << "Integer vector remainders (s+u): " << vec_remainders << endl;
}
