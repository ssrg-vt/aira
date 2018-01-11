/*****************************************************************************/
/* Implementation of BitwiseFeature class                                    */
/*****************************************************************************/

#include <string>
#include <iostream>
#include <llvm/IR/Function.h>

#include "BitwiseFeature.h"

using namespace std;
using namespace llvm;

BitwiseFeature::BitwiseFeature() :
		lshifts(0),
		rshifts(0),
		ands(0),
		ors(0),
		xors(0),
		vec_lshifts(0),
		vec_rshifts(0),
		vec_ands(0),
		vec_ors(0),
		vec_xors(0) {}

void BitwiseFeature::clear()
{
	lshifts = 0;
	rshifts = 0;
	ands = 0;
	ors = 0;
	xors = 0;
	vec_lshifts = 0;
	vec_rshifts = 0;
	vec_ands = 0;
	vec_ors = 0;
	vec_xors = 0;
}

void BitwiseFeature::collect(Instruction& instr, double bb_count)
{
	if(instr.getNumOperands() < 1)
		return;

	unsigned long opcode = instr.getOpcode();
	if(isVectorType(instr.getOperand(0)))
	{
		switch(opcode)
		{
		case Instruction::Shl:
			vec_lshifts += (unsigned int)bb_count;
			break;
		case Instruction::LShr:
		case Instruction::AShr:
			vec_rshifts += (unsigned int)bb_count;
			break;
		case Instruction::And:
			vec_ands += (unsigned int)bb_count;
			break;
		case Instruction::Or:
			vec_ors += (unsigned int)bb_count;
			break;
		case Instruction::Xor:
			vec_xors += (unsigned int)bb_count;
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
		case Instruction::Shl:
			lshifts += (unsigned int)bb_count;
			break;
		case Instruction::LShr:
		case Instruction::AShr:
			rshifts += (unsigned int)bb_count;
			break;
		case Instruction::And:
			ands += (unsigned int)bb_count;
			break;
		case Instruction::Or:
			ors += (unsigned int)bb_count;
			break;
		case Instruction::Xor:
			xors += (unsigned int)bb_count;
			break;
		default:
			//Nothing for now...
			break;
		}
	}
}

void BitwiseFeature::print(ostream& stream)
{
	stream << "Shifts (left): " << lshifts << endl;
	stream << "Shifts (right, a+l): " << rshifts << endl;
	stream << "Bitwise Ands: " << ands << endl;
	stream << "Bitwise Ors: " << ors << endl;
	stream << "Bitwise Xors: " << xors << endl;
	stream << "Vector Shifts (left): " << vec_lshifts << endl;
	stream << "Vector Shifts (right, a+l): " << vec_rshifts << endl;
	stream << "Vector Bitwise Ands: " << vec_ands << endl;
	stream << "Vector Bitwise Ors: " << vec_ors << endl;
	stream << "Vector Bitwise Xors: " << vec_xors << endl;
}
