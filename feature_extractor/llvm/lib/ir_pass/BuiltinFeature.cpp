/*****************************************************************************/
/* Implementation of BuiltinFeature class                                    */
/*****************************************************************************/

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/IntrinsicInst.h>

#include "BuiltinFeature.h"

using namespace std;
using namespace llvm;

const char* BuiltinFeature::builtin_names[] = {
	"sqrt",
	"pow",
	"sin",
	"cos",
	"exp",
	"log",
	"fma",
	"fmuladd",
	"fabs",
	"floor",
	"ceil",
	"trunc",
	"round",
};

BuiltinFeature::BuiltinFeature() :
		builtins(0),
		vec_builtins(0) {}

void BuiltinFeature::clear()
{
	builtins = 0;
	vec_builtins = 0;
}

bool BuiltinFeature::findIn(string& first, string& second)
{
	if(first.find(second) != string::npos)
		return true;
	else
		return false;
}

void BuiltinFeature::collect(Instruction& instr, double bb_count)
{
	if(isa<CallInst>(instr))
	{
		CallInst& call_instr = cast<CallInst>(instr);
		Function* func = cast<CallInst>(instr).getCalledFunction();
		if(!func)
			return;

		string name = func->getName().str();
		for(unsigned long i = 0;
				i < (sizeof(BuiltinFeature::builtin_names) / sizeof(const char*));
				i++)
		{
			string cur_name(BuiltinFeature::builtin_names[i]);
			if(findIn(name, cur_name) == true)
			{
				if(isVectorType(call_instr.getArgOperand(0)))
					vec_builtins += (unsigned long)bb_count;
				else
					builtins += (unsigned long)bb_count;
				break;
			}
		}
	}
}

void BuiltinFeature::print(ostream& stream)
{
	stream << "Built-in functions: " << builtins << endl;
}
