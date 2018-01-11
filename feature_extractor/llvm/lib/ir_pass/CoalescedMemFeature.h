/*****************************************************************************/
/* CoalescedMemFeature class                                                 */
/*                                                                           */
/* Collects information about memory accesses between consecutive threads.   */
/*                                                                           */
/* NOTE: This feature only makes sense in a multithreaded context, such as   */
/* OpenCL.                                                                   */
/*                                                                           */
/* NOTE: This implementation assumes that anything pertaining to group ID is */
/* a constant offset of (because usually all work items in a group access an */
/* array similarly to the following:                                         */
/*                                                                           */
/*   array[get_group_id(0) * get_local_size(0) + get_local_id(0)]            */
/*****************************************************************************/

#ifndef _COALESCED_MEM_FEATURE_H
#define _COALESCED_MEM_FEATURE_H

#include <map>
#include <set>

#include "IRFeature.h"

class CoalescedMemFeature : public IRFeature
{
public:
	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

	//Allow outside code read access to values
	bool getThreadIDValue(llvm::Value* val, unsigned long* stride);

private:
	std::map<llvm::Value*, unsigned long> threadID_vals;
	std::set<llvm::Value*> known_offsets;

	unsigned long getValueAmount(llvm::Value* val);

	//Handlers for individual instruction types
	void handleGetElemPtr(llvm::Instruction& instr, double bb_count);
	void handleCall(llvm::Instruction& instr);
	void handleTypeModifier(llvm::Instruction& instr);
	void handleShiftModifier(llvm::Instruction& instr);
	void handleValueModifier(llvm::Instruction& instr);
};

#endif /* _COALESCED_MEM_FEATURE_H */
