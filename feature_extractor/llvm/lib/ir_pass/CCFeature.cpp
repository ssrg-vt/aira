/*****************************************************************************/
/* Implementation of CCFeature class                                         */
/*****************************************************************************/

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

#include "CCFeature.h"

using namespace std;
using namespace llvm;

CCFeature::CCFeature() :
		nodes(0),
		edges(0),
		exit_nodes(0) {}

void CCFeature::clear()
{
	nodes = 0;
	edges = 0;
	exit_nodes = 0;
}

/*
 * Note: we don't care about BB counts for cyclomatic complexity
 */
void CCFeature::collect(Instruction& instr, double bb_count)
{
	if(isa<TerminatorInst>(instr))
	{
		TerminatorInst& term_inst(cast<TerminatorInst>(instr));
		unsigned long num_successors = term_inst.getNumSuccessors();
		if(num_successors == 0)
			exit_nodes++;
		else
			edges += num_successors;
		nodes++;
	}
}

void CCFeature::print(ostream& stream)
{
	unsigned long cc = edges - nodes + (2 * exit_nodes);
	stream << "Cyclomatic complexity: " << cc << endl;
}
