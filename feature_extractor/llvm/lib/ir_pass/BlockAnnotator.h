/*****************************************************************************/
/* BlockAnnotator class                                                      */
/*                                                                           */
/* This class provides a template for annotating the basic blocks with       */
/* execution counts in a function.  Different implementations can annotate   */
/* each basic block with different counts; an example is a heuristic-based   */
/* approach (count == loop nest depth * 1000) vs. a profiling with exact     */
/* block counts.                                                             */
/*****************************************************************************/

#ifndef _BLOCK_ANNOTATOR_H
#define _BLOCK_ANNOTATOR_H

#include <map>
#include <llvm/IR/Function.h>

class BlockAnnotator {
public:
	virtual std::map<llvm::BasicBlock*, double>* annotate(llvm::Function& func) = 0;
};

#endif /*_BLOCK_ANNOTATOR_H */
