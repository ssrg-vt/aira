/*****************************************************************************/
/* CCFeature class                                                           */
/*                                                                           */
/* This class determines the cyclomatic complexity of a function (i.e. the   */
/* number of independent linear paths through the function), defined by the  */
/* equation:                                                                 */
/*                                                                           */
/*                              C = E - N + 2P                               */
/*                                                                           */
/* where:                                                                    */
/*   C = dyclomatic complexity                                               */
/*   E = number of edges in the control flow graph (CFG)                     */
/*   N = number of nodes in the CFG                                          */
/*   P = number of connected components (exit nodes)                         */
/*****************************************************************************/

#ifndef _CC_FEATURE_H
#define _CC_FEATURE_H

#include "IRFeature.h"

class CCFeature : public IRFeature {
public:
	CCFeature();

	virtual void clear();
	virtual void collect(llvm::Instruction& instr, double bb_count);
	virtual void print(std::ostream& stream);

private:
	unsigned long nodes;
	unsigned long edges;
	unsigned long exit_nodes;
};

#endif /* _CC_FEATURE_H */
