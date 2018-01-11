#include "rose.h"
#include "common.h"
#include "sched_call_builder.h"
#include <sstream>

#define VARNAME "__kernel_features__" //"__arch_info__"
#define TYPE "kernel_features" //"__arch_info"
#define TOOL "SchedulerCallBuilder"

#define NUM_FEATURES 32

/*
 * Build a reference to the indicated variable's specified field
 *
 * TODO this is really hacky, try doing it "right" using:
 * 	-buildBinaryExpression (buildDotExp)
 *	-accessing member declarations of kernel_features struct (requires
 *		preprocessing "scheduler.h")
 */
static SgVarRefExp* buildFieldRef(SgVariableDeclaration* kernelFeatures,
	enum Field field)
{
	string name = VARNAME;
	name += ".";
	switch(field)
	{
	case INSTR_STATIC:
		name += "instr_static";
		break;
	case INSTR_DYNAMIC:
		name += "instr_dynamic";
		break;
	case SI_STATIC:
		name += "si_static";
		break;
	case SI_DYNAMIC:
		name += "si_dynamic";
		break;
	case VI_STATIC:
		name += "vi_static";
		break;
	case VI_DYNAMIC:
		name += "vi_dynamic";
		break;
	case SF_STATIC:
		name += "sf_static";
		break;
	case SF_DYNAMIC:
		name += "sf_dynamic";
		break;
	case VF_STATIC:
		name += "vf_static";
		break;
	case VF_DYNAMIC:
		name += "vf_dynamic";
		break;
	case SB_STATIC:
		name += "sb_static";
		break;
	case SB_DYNAMIC:
		name += "sb_dynamic";
		break;
	case VB_STATIC:
		name += "vb_static";
		break;
	case VB_DYNAMIC:
		name += "vb_dynamic";
		break;
	case LOAD_STATIC:
		name += "load_static";
		break;
	case LOAD_DYNAMIC:
		name += "load_dynamic";
		break;
	case STORE_STATIC:
		name += "store_static";
		break;
	case STORE_DYNAMIC:
		name += "store_dynamic";
		break;
	case CALLS_STATIC:
		name += "calls_static";
		break;
	case CALLS_DYNAMIC:
		name += "calls_dynamic";
		break;
	case MATH_STATIC:
		name += "math_static";
		break;
	case MATH_DYNAMIC:
		name += "math_dynamic";
		break;
	case CYCLOMATIC:
		name += "cyclomatic";
		break;
	case BRANCH_STATIC:
		name += "branch_static";
		break;
	case BRANCH_DYNAMIC:
		name += "branch_dynamic";
		break;
	case JUMP_STATIC:
		name += "jump_static";
		break;
	case JUMP_DYNAMIC:
		name += "jump_dynamic";
		break;
	case PARALLEL_STATIC:
		name += "parallel_static";
		break;
	case PARALLEL_DYNAMIC:
		name += "parallel_dynamic";
		break;
	case MEMORY_TX:
		name += "memory_tx";
		break;
	case MEMORY_RX:
		name += "memory_rx";
		break;
	case WORK_ITEMS:
		name += "work_items";
		break;
	default:
		ERROR(TOOL, "Attempted to set a field that doesn't exist!");
		return NULL;
	}
	return buildVarRefExp(name, getScope(kernelFeatures));
}

/*
 * Helper function to parse features from line w/ multiple features.
 */
static void getFeatures(string& line, float* features)
{
	list<string> tokens = tokenize(line, ' ');
	int size = tokens.size(), i;
	ROSE_ASSERT(size >= 3);

	list<string>::const_iterator tokenIt = tokens.end();
	tokenIt--;
	for(i = 0; i < 3; i++)
	{
		features[2-i] = atof((*tokenIt).c_str());
		tokenIt--;
	}
}

/*
 * Helper function to parse features from line w/ a single feature.
 */
static void getFeature(string& line, float* feature)
{
	list<string> tokens = tokenize(line, ' ');
	list<string>::const_iterator tokenIt = tokens.end();
	tokenIt--;
	feature[0] = atof((*tokenIt).c_str());
}

/*
 * Accumulate features from the specified file into the specified features
 * array.
 */
static bool accumulateFeaturesFromFile(const string& filename, float* features)
{
	string line;
	int i;
	float lineFeatures[3];

	ifstream infile(filename.c_str());
	if(!infile.is_open())
		return false;

	//Get rid of header
	for(i = 0; i < 3; i++)
		getline(infile, line);

	//General program characteristics
	for(i = 0; i < 11; i++)
	{
		getline(infile, line);
		getFeatures(line, lineFeatures);
		features[i*2] += lineFeatures[0];
		features[i*2 + 1] += lineFeatures[1];
	}

	//More garbage
	for(i = 0; i < 4; i++)
		getline(infile, line);

	//Control flow
	getline(infile, line);
	getFeature(line, lineFeatures);
	features[22] += lineFeatures[0];
	for(i = 23; i < 27; i += 2)
	{
		getline(infile, line);
		getFeatures(line, lineFeatures);
		features[i] += lineFeatures[0];
		features[i + 1] += lineFeatures[1];
	}

	//Last garbage
	for(i = 0; i < 2; i++)
		getline(infile, line);

	//Last features
	getline(infile, line);
	getFeatures(line, lineFeatures);
	features[27] += lineFeatures[0];
	features[28] += lineFeatures[1];

	infile.close();
	return true;
}

namespace SchedCallBuilder
{

/*
 * Build and return a variable declaration for the kernel features struct.
 */
//SgVariableDeclaration* buildArchInfo(SgScopeStatement* scope)
SgVariableDeclaration* buildKernelFeatures(SgScopeStatement* scope)
{
	SgName name(VARNAME);
	SgType* type = buildOpaqueType(TYPE, getGlobalScope(scope));

	return buildVariableDeclaration(name, type, NULL, scope);
}

/*
 * Initialize a kernel features struct from the indicated file.
 */
bool initializeFeaturesFromFiles(SgVariableDeclaration* features,
	list<string>& filenames, SgScopeStatement* scope)
{
	string msg;
	if(filenames.size() == 0)
	{
		msg = "No appropriate feature files found - did you move/rename them \
from feature extractor defaults?";
		WARNING(TOOL, msg);
		return false;
	}

	float rawFeatures[NUM_FEATURES];
	int i;
	for(i = 0; i < NUM_FEATURES;i++)
		rawFeatures[i] = 0.0;

	//Iterate through all feature files & accumulate
	list<string>::const_iterator fileIt = filenames.begin();
	i = 0;
	for(; fileIt != filenames.end(); fileIt++)
	{
		if(!accumulateFeaturesFromFile(*fileIt, rawFeatures))
		{
			msg = "\t\t\tCould not read features from file " + (*fileIt) + "!";
			ERROR(TOOL, msg);
			continue;
		}
		msg = "\t\t\t--> " + stripPathFromFileName(*fileIt);
		DEBUG(TOOL, msg);
		i++;
	}

	//TODO better way to do this?
	appendStatement(setField(features, buildFloatVal(rawFeatures[0]), INSTR_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[1]), INSTR_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[2]), SI_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[3]), SI_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[4]), VI_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[5]), VI_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[6]), SF_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[7]), SF_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[8]), VF_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[9]), VF_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[10]), SB_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[11]), SB_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[12]), VB_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[13]), VB_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[14]), LOAD_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[15]), LOAD_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[16]), STORE_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[17]), STORE_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[18]), CALLS_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[19]), CALLS_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[20]), MATH_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[21]), MATH_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[22]), CYCLOMATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[23]), BRANCH_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[24]), BRANCH_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[25]), JUMP_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[26]), JUMP_DYNAMIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[27]), PARALLEL_STATIC), scope);
	appendStatement(setField(features, buildFloatVal(rawFeatures[28]), PARALLEL_DYNAMIC), scope);

	if(i > 0)
		return true;
	else
	{
		msg = "\t\tCould not initialize features struct!";
		ERROR(TOOL, msg);
		return false;
	}
}

/*
 * Assign the specified field an expression.
 */
SgExprStatement* setField(SgVariableDeclaration* kernelFeatures, SgExpression* rhs,
	enum Field field)
{
	SgVarRefExp* varRef = buildFieldRef(kernelFeatures, field);
	return buildAssignStatement(varRef, rhs);
}

/*
 * Add an expression to the specified field.
 */
SgExprStatement* addToField(SgVariableDeclaration* kernelFeatures, SgExpression* rhs,
	enum Field field)
{
	SgVarRefExp* varRef = buildFieldRef(kernelFeatures, field);
	SgAddOp* addOp = buildAddOp(varRef, rhs);
	varRef = buildFieldRef(kernelFeatures, field);
	return buildAssignStatement(varRef, addOp);
}

/*
 * Build a call to the scheduler, which selects an implementation for the
 * kernel.
 */
SgExprStatement* buildSchedulerCall(SgVariableDeclaration* partitionNum,
	SgVariableDeclaration* kernelFeatures)
{
	ROSE_ASSERT(partitionNum != NULL);
	ROSE_ASSERT(kernelFeatures != NULL);

	SgName name("select_implementation");
	SgType* type = buildUnsignedLongType();
	SgExprListExp* params = buildExprListExp(
		buildAddressOfOp(buildVarRefExp(kernelFeatures)));
	SgFunctionCallExp* funcCall = buildFunctionCallExp(name, type, params,
		getScope(kernelFeatures));

	return buildAssignStatement(buildVarRefExp(partitionNum), funcCall);
}

/*
 * Build a call to cleanup after a call to the scheduler.  This informs the
 * scheduler that the kernel has finished executing on a resource and that it
 * can perform any appropriate cleanup.
 */
SgExprStatement* buildCleanupCall(SgVariableDeclaration* kernelFeatures)
{
	ROSE_ASSERT(kernelFeatures != NULL);

	SgName name("cleanup_kernel");
	SgType* type = buildVoidType();
	SgExprListExp* params = buildExprListExp(
		buildAddressOfOp(buildVarRefExp(kernelFeatures)));
	SgFunctionCallExp* funcCall = buildFunctionCallExp(name, type, params,
		getScope(kernelFeatures));

	return buildExprStatement(funcCall);
}

}
