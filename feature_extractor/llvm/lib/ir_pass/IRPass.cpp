/*****************************************************************************/
/* Implementation of IRPass class                                            */
/*****************************************************************************/

#include <iostream>
#include <fstream>
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

#include "FunctionIRFeatures.h"
#include "IRPass.h"

/* Basic block execution counts */
#include <llvm/Analysis/LoopInfo.h>
#include "BlockAnnotator.h"
#include "HeuristicAnnotator.h"

using namespace std;
using namespace llvm;

bool IRPass::doInitialization(Module &M)
{
	cout << "Collecting features from the IR...\n";
	return true;
}

void IRPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.addRequired<LoopInfo>();
}

bool IRPass::runOnModule(Module &M)
{
	//Iterate over all functions
	for(Module::iterator mi = M.begin(); mi != M.end(); mi++)
	{
		//Avoid non-defined functions like printf
		Function& F = *mi;
		if(F.isDeclaration())
			continue;

		FunctionIRFeatures features(F);
		cout << " -> " << F.getName().str() << endl;

		//TODO add other block annotators?
		map<BasicBlock*, double>* bb_counts = NULL;
		LoopInfo& li = getAnalysis<LoopInfo>(F);
		HeuristicAnnotator annotator(li);
		bb_counts = annotator.annotate(F);

		//Iterate over all basic blocks
		Function::iterator f_it;
		for(f_it = F.begin(); f_it != F.end(); ++f_it)
		{
			//Iterate over all instructions in the basic block
			double cur_count = bb_counts->find(f_it)->second;
			BasicBlock& bb = *f_it;
			BasicBlock::iterator bb_it;
			for(bb_it = bb.begin(); bb_it != bb.end(); ++bb_it)
				features.extract(*bb_it, cur_count);
		}

		//Write to file & cleanup
		string filename(F.getName().str() + ".feat");
		ofstream fstream(filename.c_str());
		features.print(fstream);
		features.clear();
		fstream.close();
		delete bb_counts;
	}

	//The function is not modified, return false
	return false;
}

bool IRPass::doFinalization(Module &M)
{
	cout << "Finished collecting IR features" << endl;
	return true;
}

char IRPass::ID = 0;
static RegisterPass<IRPass> P("irfeatures", "IR feature extractor for OpenCL Kernels", false, false);
