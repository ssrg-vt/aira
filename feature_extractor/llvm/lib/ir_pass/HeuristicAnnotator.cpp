/*****************************************************************************/
/* Implementation for HeuristicAnnotator class                               */
/*****************************************************************************/

#include <iostream>
#include <cmath>
#include <assert.h>
#include <llvm/Support/CFG.h>
#include <llvm/IR/Instructions.h>

#include "HeuristicAnnotator.h"

using namespace std;
using namespace llvm;

/*
 * Values used to estimate the number of times the function is called & the
 * number of times each loop executes.
 */
const unsigned long HeuristicAnnotator::entry_count = 10000;
const unsigned long HeuristicAnnotator::loop_exec_count = 1000;

/*
 * Constructor - stores reference to LoopInfo obtained from previous analysis
 * pass.
 */
HeuristicAnnotator::HeuristicAnnotator(LoopInfo& p_li) : li(p_li) {}

/*
 * Returns true if all predecessors have been visited, false otherwise.
 */
bool HeuristicAnnotator::allPredecessorsVisited(llvm::BasicBlock* block,
		set<BasicBlock*>& visited)
{
	//If loop header, then by the algorithm all predecessors have been visited.
	//This check breaks cycles, allowing values to flow forward.
	if(li.isLoopHeader(block))
		return true;

	for(pred_iterator pi = pred_begin(block); pi != pred_end(block); pi++)
	{
		if(visited.find(*pi) == visited.end())
			return false;
	}
	return true;
}

/*
 * Returns the number of true successors, indicated by non-back CFG edges.
 */
unsigned int HeuristicAnnotator::numSuccessors(BasicBlock* block,
		const set<BasicBlock*>& visited)
{
	unsigned int count = 0;
	for(succ_iterator si = succ_begin(block); si != succ_end(block); si++)
	{
		if(visited.find(*si) == visited.end())
			count++;
	}
	if(count == 0) //No successors, avoid divide-by-zero
		return 1;
	else
		return count;
}


/*
 * Update a basic block's successors with execution counts.
 *
 * TODO if successor is a loop header, need to add cur_count to the count for
 *      the basic block at the end of the loop
 */
void HeuristicAnnotator::updateSuccessors(map<BasicBlock*, double>* bb_counts,
		list<BasicBlock*>& work_q, set<BasicBlock*>& visited, BasicBlock* cur,
		double cur_count)
{
	cur_count /= numSuccessors(cur, visited);
	unsigned int cur_depth = li.getLoopDepth(cur);
	for(succ_iterator si = succ_begin(cur); si != succ_end(cur); si++)
	{
		if(visited.find(*si) == visited.end()) //Not previously visited, not a loop
		{
			if(bb_counts->find(*si) != bb_counts->end())
				bb_counts->find(*si)->second +=
					(cur_count * pow(loop_exec_count, (int)li.getLoopDepth(*si) - (int)cur_depth));
			else
			{
				bb_counts->insert(pair<BasicBlock*, double>(*si,
					cur_count * pow(loop_exec_count, (int)li.getLoopDepth(*si) - (int)cur_depth)));
				work_q.push_back(*si);
			}
		}
//		else //Part of a loop, scale all blocks in loop
//			updateLoop(*si, cur, bb_counts);
	}
}

/*
 * Scale basic block counts inside of a loop by loop_exec_count
 */
/*void HeuristicAnnotator::updateLoop(BasicBlock* begin, BasicBlock* end,
		map<BasicBlock*, int>* bb_counts)
{
	BasicBlock* cur;
	list<BasicBlock*> work_q;

	bb_counts->find(cur)->second *= loop_exec_count;
	work_q.push_back(begin);

	//Iterate through all blocks in the loop & scale counts accordingly
	while(work_q.size() > 0)
	{
		cur = work_q.front();
		work_q.pop_front();

		if(cur == end)
			continue;
		else
		{	
			//Check for continue + break
			BranchInst* br = NULL;
			bool continueOrBreak = false;
			if(isa<BranchInst>(cur->getTerminator()))
				br = cast<BranchInst>(cur->getTerminator());
			if(br != NULL)
			{
				for(unsigned int i = 0; i < br->getNumSuccessors(); i++)
					if(br->getSuccessor(i) == begin || br->getSuccessor(i) == end)
						continueOrBreak = true;
				if(continueOrBreak)
					continue;
			}

			//Adjust successors
			for(succ_iterator si = succ_begin(cur); si != succ_end(cur); si++)
			{
				bb_counts->find(*si)->second *= loop_exec_count;
				work_q.push_back(*si);
			}
		}
	}
}*/

/*
 * Annotate basic blocks with execution counts using heuristics described
 * previously.
 *
 * NOTE: This assumes regular control flow in the application, i.e. no gotos
 */
map<BasicBlock*, double>* HeuristicAnnotator::annotate(Function& func)
{
	map<BasicBlock*, double>* bb_counts = new map<BasicBlock*, double>();
	set<BasicBlock*> visited;
	list<BasicBlock*> work_q;
	double cur_count = entry_count;

	//Add the entry node as a starting point
	BasicBlock* cur = &func.getEntryBlock();
	bb_counts->insert(pair<BasicBlock*, double>(cur, cur_count));
	visited.insert(cur);
	updateSuccessors(bb_counts, work_q, visited, cur, cur_count);

	//Iterate through all BBs in the CFG, propagating values
	while(work_q.size() > 0)
	{
		cur = work_q.front();
		work_q.pop_front();

		//Make sure all predecessors have been evaluated
		if(!allPredecessorsVisited(cur, visited))
			work_q.push_back(cur);
		else
		{
			visited.insert(cur);
			cur_count = bb_counts->find(cur)->second;
			updateSuccessors(bb_counts, work_q, visited, cur, cur_count);
		}
	}

	cout << "  1. Basic block counts (loop nest depth):" << endl;
	for(map<BasicBlock*, double>::const_iterator mi = bb_counts->begin();
			mi != bb_counts->end();
			mi++)
				cout << "      " << mi->first->getName().str() << ": " << mi->second
							<< " (" << li.getLoopDepth(mi->first) << ")" << endl;	

	return bb_counts;
}
