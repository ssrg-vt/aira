/*****************************************************************************/
/* MemoryFeature class                                                       */
/*                                                                           */
/* This class counts memory loads and stores.  It does NOT analyze memory    */
/* access patterns.                                                          */
/*****************************************************************************/

#ifndef _MEMORY_FEATURE_H
#define _MEMORY_FEATURE_H

#include "IRFeature.h"
#include "CoalescedMemFeature.h"

class MemoryFeature : public IRFeature {
public:
	MemoryFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

	static const unsigned int max_coalesced_stride = 2;

private:
	unsigned long loads;
	unsigned long stores;
	unsigned long local_loads;
	unsigned long local_stores;
	unsigned long coalesced;
	unsigned long maybe_coalesced;
	unsigned long uncoalesced;
	CoalescedMemFeature coa_tracker;

	enum type {
		LOAD,
		STORE,
		NA
	};

	bool isLocalAccess(llvm::Instruction& instr, type t);
};

#endif /* _MEMORY_FEATURE_H */
