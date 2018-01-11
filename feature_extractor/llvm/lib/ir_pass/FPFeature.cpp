/*****************************************************************************/
/* Implementation of FPFeature class                                         */
/*****************************************************************************/

#include <iostream>
#include <string>
#include <llvm/IR/Function.h>

#include "FPFeature.h"

using namespace std;
using namespace llvm;

void FPFeature::collect(Instruction& instr, double bb_count)
{
	if(instr.getNumOperands() < 1)
		return;

	unsigned int opcode = instr.getOpcode();
	if(isVectorType(instr.getOperand(0)))
	{
		switch(opcode)
		{
		case Instruction::FAdd:
			vec_adds += (unsigned int)bb_count;
			break;
		case Instruction::FSub:
			vec_subtracts += (unsigned int)bb_count;
			break;
		case Instruction::FMul:
			vec_multiplies += (unsigned int)bb_count;
			break;
		case Instruction::FDiv:
			vec_divides += (unsigned int)bb_count;
			break;
		case Instruction::FRem:
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
		case Instruction::FAdd:
			adds += (unsigned int)bb_count;
			break;
		case Instruction::FSub:
			subtracts += (unsigned int)bb_count;
			break;
		case Instruction::FMul:
			multiplies += (unsigned int)bb_count;
			break;
		case Instruction::FDiv:
			divides += (unsigned int)bb_count;
			break;
		case Instruction::FRem:
			remainders += (unsigned int)bb_count;
			break;
		default:
			//Nothing for now...
			break;
		}
	}
}

void FPFeature::print(ostream& stream)
{
	stream << "Floating-point additions: " << adds << endl;
	stream << "Floating-point subtractions: " << subtracts << endl;
	stream << "Floating-point multiplications: " << multiplies << endl;
	stream << "Floating-point divisions: " << divides << endl;
	stream << "Floating-point remainders: " << remainders << endl;
	stream << "Floating-point vector additions: " << vec_adds << endl;
	stream << "Floating-point vector subtractions: " << vec_subtracts << endl;
	stream << "Floating-point vector multiplications: " << vec_multiplies << endl;
	stream << "Floating-point vector divisions: " << vec_divides << endl;
	stream << "Floating-point vector remainders: " << vec_remainders << endl;
}
