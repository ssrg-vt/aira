/*****************************************************************************/
/* Implementation of CoalescedMemFeature class                               */
/*****************************************************************************/

#include <iostream>
#include <assert.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

#include "CoalescedMemFeature.h"

using namespace std;
using namespace llvm;

//TODO make sure that all types of value propagation (especially constant)
//have been performed

/*
 * Clear all collected data, thread ID values & known offsets.
 */
void CoalescedMemFeature::clear()
{
	threadID_vals.clear();
	known_offsets.clear();
}

/*
 * Based on instruction type & operands, collect values that are dependent
 * on thread ID and use those to determine whether accesses are coalesced or
 * not.
 */
void CoalescedMemFeature::collect(Instruction& instr, double bb_count)
{
	unsigned int opcode = instr.getOpcode();
	switch(opcode)
	{
	case Instruction::Call:
		handleCall(instr);
		break;
	case Instruction::GetElementPtr:
		handleGetElemPtr(instr, bb_count);
		break;
	case Instruction::Trunc:
	case Instruction::ZExt:
	case Instruction::SExt:
		handleTypeModifier(instr);
		break;
	case Instruction::AShr:
	case Instruction::LShr:
	case Instruction::Shl:
		handleShiftModifier(instr);
		break;
	case Instruction::Add:
	case Instruction::Sub:
	case Instruction::Mul:
	case Instruction::UDiv:
	case Instruction::SDiv:
	case Instruction::And:
	case Instruction::Or:
	case Instruction::Xor:
		handleValueModifier(instr);
		break;
	default:
		//Nothing for now...
		break;
	}
}

void CoalescedMemFeature::print(ostream& stream)
{
	return;
}

/*
 * Public access to currently parsed values.  If it pertains to the thread ID,
 * return true, else return false.
 */
bool CoalescedMemFeature::getThreadIDValue(Value* val, unsigned long* stride)
{
	if(threadID_vals.find(val) != threadID_vals.end())
	{
		*stride = threadID_vals[val];
		return true;
	}
	else
		return false;
}

/*
 * Convert a value object into an amount describing coalescing.
 */
unsigned long CoalescedMemFeature::getValueAmount(Value* val)
{
	unsigned long amount = 0;
	if(isa<ConstantInt>(val))
		amount = cast<ConstantInt>(val)->getSExtValue();
	else if(known_offsets.find(val) != known_offsets.end())
		amount = 1;
	else if(threadID_vals.find(val) != threadID_vals.end())
		amount = threadID_vals[val];
	else
		amount = 0;
	return amount;
}

///////////////////////////////////////////////////////////////////////////////
// Handlers for individual instruction types
///////////////////////////////////////////////////////////////////////////////

/*
 * Check operands of the getelementptr instruction to determine if generated
 * pointer utilizes thread ID, and if so, what type of access is performed.
 */
void CoalescedMemFeature::handleGetElemPtr(llvm::Instruction& instr,
		double bb_count)
{
	//TODO handle case where there are > 2 operands
	if(threadID_vals.find(instr.getOperand(1)) != threadID_vals.end())
		threadID_vals[&instr] = threadID_vals[instr.getOperand(1)];

/*	GetElementPtrInst& gep_inst = cast<GetElementPtrInst>(instr);
	for(GetElementPtrInst::const_op_iterator op_it = gep_inst.idx_begin();
			op_it != gep_inst.idx_end();
			op_it++)
	{
		Value* op = *op_it;
		if(threadID_vals.find(op) != threadID_vals.end())
		{
			unsigned int stride = threadID_vals[op];
			if(stride == 0)
				maybe_coalesced += bb_count;
			else if(stride <= max_coalesced_stride)
				coalesced += bb_count;
			else
				uncoalesced += bb_count;
		}
	}*/
}

/*
 * Check for calls to get_global_id & get_local_id, which generate values based
 * on the thread's ID, and get_group_id, which are assumed to generate constant
 * offset values.  Array accesses that in some way use a thread ID (and NOT the
 * group ID)  are how coalesced memory accesses are determined.
 */
void CoalescedMemFeature::handleCall(Instruction& instr)
{
	Function* func = cast<CallInst>(instr).getCalledFunction();
	if(!func) //Check for non-existent function pointers
		return;

	string func_name = func->getName().str();
	if(func_name == "get_global_id" || func_name == "get_local_id")
		threadID_vals[&instr] = 1;
	else if(func_name == "get_group_id")
	{
		threadID_vals[&instr] = 1;
		known_offsets.insert(&instr);
	}
	else
	{
		//TODO parse other calls to check to see if they use that value
	}
}

/*
 * Add values that have their type modified, i.e. truncate from 64 to 32 bit.
 * Values added here maintain the same stride as the thread ID value being
 * analyzed.
 */
void CoalescedMemFeature::handleTypeModifier(Instruction& instr)
{
	Value* op = instr.getOperand(0);
	if(threadID_vals.find(op) != threadID_vals.end())
	{
		threadID_vals[&instr] = threadID_vals[op];
		if(known_offsets.find(op) != known_offsets.end())
			known_offsets.insert(&instr);
	}
}

/*
 * Add values that shift thread ID values.
 *
 * TODO this assumes that we do not care about the shift amount, at some point
 * we might be shifting by a thread ID amount
 */
void CoalescedMemFeature::handleShiftModifier(Instruction& instr)
{
	unsigned long stride = 0;
	unsigned int shift_val = 0;
	Value* op = instr.getOperand(0);

	if(threadID_vals.find(op) != threadID_vals.end())
	{
		if(known_offsets.find(op) != known_offsets.end())
		{
			stride = 1;
			known_offsets.insert(&instr);
		}
		else
		{
			shift_val = getValueAmount(instr.getOperand(1));
			switch(instr.getOpcode())
			{
			case Instruction::AShr:
			case Instruction::LShr:
				stride = threadID_vals[op] >> shift_val;
				break;
			case Instruction::Shl:
				stride = threadID_vals[op] << shift_val;
				break;
			}
		}

		threadID_vals[&instr] = stride;
	}
}

/*
 * Add values that perform some math operation on thread ID values.
 */
void CoalescedMemFeature::handleValueModifier(Instruction& instr)
{
	unsigned long stride = 0;
	unsigned long lval = 0;
	unsigned long rval = 0;
	bool lval_isConstant = false;
	bool rval_isConstant = false;
	Value* lop = instr.getOperand(0);
	Value* rop = instr.getOperand(1);

	if(threadID_vals.find(lop) != threadID_vals.end() ||
			threadID_vals.find(rop) != threadID_vals.end())
	{
		//At least one of the vals depends on thread ID, get vals for each operand
		lval = getValueAmount(lop);
		rval = getValueAmount(rop);

		if(isa<ConstantInt>(lop) ||
				(known_offsets.find(lop) != known_offsets.end()))
			lval_isConstant = true;
		if(isa<ConstantInt>(rop) ||
				(known_offsets.find(rop) != known_offsets.end()))
			rval_isConstant = true;

		//Multiply/divide change stride regardless of the constant-ness of the
		//operands.  Addition and subtraction only change stride if both operands
		//are based on the thread ID - addition/subtraction by a contsant are just
		//offsets and do not affect coalescing.  If either operand has an unknown
		//stride, then the resulting stride is unknown.
		if(lval == 0 || rval == 0)
			stride = 0;
		else if(lval_isConstant && rval_isConstant)
		{
			stride = 1;
			if(known_offsets.find(lop) != known_offsets.end() ||
					known_offsets.find(rop) != known_offsets.end())
				known_offsets.insert(&instr);
		}
		else
		{
			switch(instr.getOpcode())
			{
			case Instruction::Add:
				if(lval_isConstant)
					stride = rval;
				else if(rval_isConstant)
					stride = lval;
				else
					stride = lval + rval;
				break;
			case Instruction::Sub:
				if(lval_isConstant)
					stride = rval;
				else if(rval_isConstant)
					stride = lval;
				else
					stride = lval - rval;
				break;
			case Instruction::Mul:
				stride = lval * rval;
				break;
			case Instruction::UDiv:
			case Instruction::SDiv:
				stride = lval / rval;
				break;
			case Instruction::And:
				stride = lval & rval;
				break;
			case Instruction::Or:
				stride = lval | rval;
				break;
			case Instruction::Xor:
				stride = lval ^ rval;
				break;
			}
		}

		threadID_vals[&instr] = stride;
	}
}

