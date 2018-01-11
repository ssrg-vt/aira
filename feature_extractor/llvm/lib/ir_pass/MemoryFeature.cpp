/*****************************************************************************/
/* Implementation of MemoryFeature class                                     */
/*****************************************************************************/

#include <iostream>
#include <llvm/IR/Function.h>

#include "MemoryFeature.h"

#define LOCAL_MEMSPACE 3

using namespace std;
using namespace llvm;

MemoryFeature::MemoryFeature() :
		loads(0),
		stores(0),
		local_loads(0),
		local_stores(0),
		coalesced(0),
		maybe_coalesced(0),
		uncoalesced(0) {}

void MemoryFeature::clear()
{
	loads = 0;
	stores = 0;
	local_loads = 0;
	local_stores = 0;
	coalesced = 0;
	maybe_coalesced = 0;
	uncoalesced = 0;
	coa_tracker.clear();
}

/*
 * Return whether or not the pointer being dereferenced points to memory in the
 * local OpenCL memory space or not.
 */
bool MemoryFeature::isLocalAccess(Instruction& instr, enum type t)
{
	Value* op = NULL;
	if(t == LOAD)
		op = instr.getOperand(0);
	else
		op = instr.getOperand(1);

	if(op->getType()->getPointerAddressSpace() == LOCAL_MEMSPACE)
		return true;
	else
		return false;
}

/*
 * Collect memory features pertaining to loads & stores
 */
void MemoryFeature::collect(Instruction& instr, double bb_count)
{
	coa_tracker.collect(instr, bb_count);

	unsigned int opcode = instr.getOpcode();
	enum type t = NA;
	switch(opcode)
	{
	case Instruction::ExtractElement:
	case Instruction::ExtractValue:
	case Instruction::Load:
		t = LOAD;
		loads += (unsigned long)bb_count;
		if(isLocalAccess(instr, t))
			local_loads += (unsigned long)bb_count;
		break;
	case Instruction::InsertElement:
	case Instruction::InsertValue:
	case Instruction::Store:
		t = STORE;
		stores += (unsigned long)bb_count;
		if(isLocalAccess(instr, t))
			local_stores += (unsigned long)bb_count;
		break;
	default:
		//Nothing for now...
		break;
	}

	//Handle memory coalescing checks
	unsigned long stride;
	Value* val;
	if(t == LOAD)
		val = instr.getOperand(0);
	else if(t == STORE)
		val = instr.getOperand(1);
	else
		return;

	if(coa_tracker.getThreadIDValue(val, &stride))
	{
		if(stride == 0)
			maybe_coalesced += (unsigned long)bb_count;
		else if(stride <= max_coalesced_stride)
			coalesced += (unsigned long)bb_count;
		else
			uncoalesced += (unsigned long)bb_count;
	}
}

void MemoryFeature::print(ostream& stream)
{
	stream << "Loads: " << loads << endl;
	stream << "Stores: " << stores << endl;
	stream << "Local-memory loads: " << local_loads << endl;
	stream << "Local-memory stores: " << local_stores << endl;
	stream << "Coalesced accesses: " << coalesced << endl;
	stream << "Maybe-coalesced accesses: " << maybe_coalesced << endl;
	stream << "Uncoalesced accesses: " << uncoalesced << endl;
}
