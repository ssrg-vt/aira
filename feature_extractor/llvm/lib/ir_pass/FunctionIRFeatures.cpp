#include <string>
#include <llvm/IR/Function.h>

#include "BitwiseFeature.h"
#include "BuiltinFeature.h"
#include "CallFeature.h"
#include "CCFeature.h"
#include "CoalescedMemFeature.h"
#include "ControlFlowFeature.h"
#include "FPFeature.h"
#include "InstrFeature.h"
#include "IntFeature.h"
#include "MemoryFeature.h"

#include "FunctionIRFeatures.h"

using namespace std;
using namespace llvm;

FunctionIRFeatures::FunctionIRFeatures(Function& F)
{
	/////////////////////////////////////////////////////////////////////////////
	//NOTE: Add new features to be collected here!
	/////////////////////////////////////////////////////////////////////////////

	//Instruction count feature
	features.push_back(new InstrFeature());

	//Basic math (floating-point) features
	features.push_back(new FPFeature());

	//Basic math integer features) features
	features.push_back(new IntFeature());

	//Bitwise features
	features.push_back(new BitwiseFeature());

	//Memory load+store features
	features.push_back(new MemoryFeature());

	//Coalesced memory features
	features.push_back(new CoalescedMemFeature());

	//Function call features
	features.push_back(new CallFeature());

	//Built-in function features
	features.push_back(new BuiltinFeature());

	//Control-flow features
	features.push_back(new ControlFlowFeature());

	//Cyclomatic complexity features
	features.push_back(new CCFeature());
}

FunctionIRFeatures::~FunctionIRFeatures()
{
	vector<IRFeature*>::iterator it;
	for(it = features.begin(); it != features.end(); ++it)
		delete *it;
}

void FunctionIRFeatures::clear()
{
	vector<IRFeature*>::iterator it;
	for(it = features.begin(); it != features.end(); ++it)
		(*it)->clear();
}

void FunctionIRFeatures::extract(Instruction& instr, double bb_count)
{
	//Allow each feature object to collect whatever features it needs from the
	//instruction
	vector<IRFeature*>::iterator it;
	for(it = features.begin(); it != features.end(); ++it)
		(*it)->collect(instr, bb_count);
}

void FunctionIRFeatures::print(ostream& stream)
{
	vector<IRFeature*>::iterator it;
	for(it = features.begin(); it != features.end(); ++it)
		(*it)->print(stream);
}

