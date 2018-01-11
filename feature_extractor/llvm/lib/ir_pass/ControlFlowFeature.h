/*****************************************************************************/
/* ControlFlowFeature class                                                  */
/*                                                                           */
/* Gathers features pertaining to control flow in the IR.                    */
/*****************************************************************************/

#ifndef _CONTROL_FLOW_FEATURE_H
#define _CONTROL_FLOW_FEATURE_H

#include "IRFeature.h"

class ControlFlowFeature : public IRFeature {
public:
	ControlFlowFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

private:
	unsigned long cond_branches;
	unsigned long uncond_branches;
	unsigned long switches;
};

#endif /* _CONTROL_FLOW_FEATURE_H */

