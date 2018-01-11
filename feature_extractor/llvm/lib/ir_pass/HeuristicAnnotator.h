/*****************************************************************************/
/* HeuristicAnnotator class                                                  */
/*                                                                           */
/* Uses simple heuristics on the call-graph to estimate basic block counts.  */
/*                                                                           */
/* TODO add description of heuristic                                         */
/*                                                                           */
/* Note: using optimization like -O2 helps eliminate complex control flow    */
/*       that can mess up the heuristics basic block counts!                 */
/*                                                                           */
/* Known issues:                                                             */
/* 1. If no optimizations are used, clang will emit a generic loop with      */
/*    condition checking at the beginning, rather than the end (which is     */
/*    usually bad for branch predictors if the loop executes multiple        */
/*    iterations).  Besides this performance drawback, it causes issues in   */
/*    the heuristic, based on the way the algorithm is constructed. Consider */
/*    the following example:                                                 */
/*                                                                           */
/*                - for.cond -                                               */
/*               /     |      \                                              */
/*              |      |      /                                              */
/*              |   for.body -                                               */
/*               \                                                           */
/*                - for.end                                                  */
/*                                                                           */
/*    Based on the heuristic described above, if for.cond executes 10000     */
/*    times, then for.body executes 5000 times and for.end executes 5 times  */
/*    (loop nest depth causes it to be scaled down by 1000).  However, we    */
/*    know that because for.cond is a loop header, all counts evenutally go  */
/*    to for.end, meaning that for.end should be 10.  Using optimizations    */
/*    usually fixes this.                                                    */
/*****************************************************************************/

#ifndef _HEURISTIC_ANNOTATOR_H
#define _HEURISTIC_ANNOTATOR_H

#include "BlockAnnotator.h"

#include <list>
#include <set>

#include <llvm/Analysis/LoopInfo.h>

class HeuristicAnnotator : public BlockAnnotator {
public:
	HeuristicAnnotator(llvm::LoopInfo& p_li);
	virtual std::map<llvm::BasicBlock*, double>* annotate(llvm::Function& func);

	static const unsigned long entry_count;
	static const unsigned long loop_exec_count;

private:
	llvm::LoopInfo& li;

	bool allPredecessorsVisited(llvm::BasicBlock* block,
			std::set<llvm::BasicBlock*>& visited);
	unsigned int numSuccessors(llvm::BasicBlock* block,
			const std::set<llvm::BasicBlock*>& visited);
	void updateSuccessors(std::map<llvm::BasicBlock*, double>* bb_counts,
			std::list<llvm::BasicBlock*>& work_q, std::set<llvm::BasicBlock*>& visited,
			llvm::BasicBlock* cur, double cur_count);
//	void updateLoop(llvm::BasicBlock* begin, llvm::BasicBlock* end,
//			std::map<llvm::BasicBlock*, int>* bb_counts);
};

#endif /* _HEURISTIC_ANNOTATOR_H */
