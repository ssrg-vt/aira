/*
 * call_scheduler.cpp
 *
 *  Created on: Jun 7, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include <dirent.h>

#include "common.h"
#include "add_scheduler_calls_common.h"
#include "pragma_handling.h"
#include "call_scheduler.h"
#include "mm_call_builder.h"
#include "sched_call_builder.h"
#include "miscellaneous.h"

/*
 * Default folder that contains all features
 */
string CallScheduler::featuresFolder = DEF_FEATURES_FOLDER;
bool CallScheduler::defaultFeaturesFolder = true;

/*
 * Default constructor.
 */
CallScheduler::CallScheduler(SgFunctionDeclaration* p_function, Pragmas& p_pragmas) :
	function(p_function),
	pragmas(p_pragmas),
	switchStmt(NULL),
	partNum(NULL),
	kernelFeatures(NULL)
{
	ROSE_ASSERT(function);
	string msg = "Adding scheduler calls to function " + NAME(function);
	DEBUG(TOOL, msg);

	//Find the switch statement, before which all new code will be inserted
	//TODO find a more concrete/safe way to insert code?
	Rose_STL_Container<SgNode*> nodes = querySubTree(function, V_SgSwitchStatement);
	Rose_STL_Container<SgNode*>::const_iterator nodeIt = nodes.begin();
	for(nodeIt = nodes.begin(); nodeIt != nodes.end(); nodeIt++)
	{
		if(isSgSwitchStatement(*nodeIt))
			switchStmt = isSgSwitchStatement(*nodeIt);
	}
	ROSE_ASSERT(switchStmt != NULL);
}

/*
 * Set the directory where feature files are stored.
 */
void CallScheduler::setFeaturesFolder(string p_featuresFolder)
{
	featuresFolder = p_featuresFolder;
	defaultFeaturesFolder = false;
}

/*
 * Add calls to initialize all information and make the necessary calls to the
 * runtime scheduler to select an architecture.
 */
void CallScheduler::addSchedulerCalls()
{
	addStructInitialization();
	addDynamicFeatures();
	addCallToScheduler();
	addCallToCleanup();
}

/*
 * Search the specified features directory for pertinent feature files.
 */
bool CallScheduler::findFeatureFiles()
{
	string msg;

	//Construct feature file name
	string appFeaturesFolder = featuresFolder;
	string sourceName = getEnclosingFileNode(function)->getFileName();
	if(sourceName.find("_arch") != string::npos)
		sourceName = sourceName.erase(sourceName.find("_arch"));
	sourceName = stripPathFromFileName(sourceName);
	if(defaultFeaturesFolder)
		appFeaturesFolder += "/" + sourceName;
	sourceName = sourceName + "_" + NAME(function) + "._omp_fn.";

	DIR* dir;
	dir = opendir(appFeaturesFolder.c_str());
	if(!dir)
	{
		msg = "Could not open directory " + appFeaturesFolder + "!";
		ERROR(TOOL, msg);
		return false;
	}

	appFeaturesFolder = getAbsolutePathFromRelativePath(appFeaturesFolder, true);
	msg = "\t\tGetting feature files from directory " + appFeaturesFolder;
	DEBUG(TOOL, msg);
	struct dirent* dirEnt;
	while((dirEnt = readdir(dir)) != NULL)
	{
		string dirName(dirEnt->d_name);
		if(dirName.find(sourceName) != string::npos)
			featureFiles.push_back(appFeaturesFolder + "/" + dirEnt->d_name);
	}
	return true;
}

/*
 * Declare & initialize the architecture information struct.
 *
 * TODO: This really only needs to be done once, find a way to initialize once
 */
void CallScheduler::addStructInitialization()
{
	string msg = "\tInitializing architecture info...";
	DEBUG(TOOL, msg);

	//Declare variables
	SgAssignInitializer* varInit = buildAssignInitializer(buildIntVal(0), buildIntType());
	SgVariableDeclaration* initInfo = buildVariableDeclaration("__init", buildIntType(), varInit,
			function->get_definition());
	setStatic(initInfo);
	insertStatement(switchStmt, initInfo);
	kernelFeatures = SchedCallBuilder::buildKernelFeatures(function->get_scope());
	setStatic(kernelFeatures);
	insertStatement(switchStmt, kernelFeatures);

	//Build if statement so initialization only occurs the first time
	SgBasicBlock* initBody = buildBasicBlock();
	SgIfStmt* ifStmt = buildIfStmt(buildNotOp(buildVarRefExp(initInfo)), initBody, NULL);
	insertStatement(switchStmt, ifStmt);

	//Get list of feature files & finish initialization
	findFeatureFiles();
	SchedCallBuilder::initializeFeaturesFromFiles(kernelFeatures, featureFiles, initBody);
	initBody->append_statement(buildAssignStatement(buildVarRefExp(initInfo), buildIntVal(1)));
}

/*
 * Tally the amount of data that needs to be transferred to and from the
 * accelerator.
 *
 * Note: this needs to be done every time the kernel is invoked, as the amount
 * of data to transfer might be different for different kernel invocations.
 */
void CallScheduler::addDynamicFeatures()
{
	string msg = "\tCalculating amount of memory transfer...";
	DEBUG(TOOL, msg);
	ROSE_ASSERT(kernelFeatures != NULL);
	SgFunctionDefinition* funcDef = function->get_definition();

	//Initialize to zero
	SgExpression* initExpr = buildIntVal(0);
	insertStatement(switchStmt, SchedCallBuilder::setField(kernelFeatures, initExpr, MEMORY_TX));
	initExpr = buildIntVal(0);
	insertStatement(switchStmt, SchedCallBuilder::setField(kernelFeatures, initExpr, MEMORY_RX));

	/*Add up number of bytes to send */
	//Add up inputs
	int scalarVals = 0;
	scalarVals += sumVarSizes(pragmas.getInputs()->getNames(), funcDef, true);
	scalarVals += sumVarSizes(pragmas.getGlobalInputs()->getNames(), funcDef, true);
	if(scalarVals > 0)
		insertStatement(switchStmt,
				SchedCallBuilder::addToField(kernelFeatures, buildIntVal(scalarVals), MEMORY_TX));

	//Add up outputs
	scalarVals = 0;
	scalarVals += sumVarSizes(pragmas.getOutputs()->getNames(), funcDef, false);
	scalarVals += sumVarSizes(pragmas.getGlobalOutputs()->getNames(), funcDef, false);
	if(scalarVals > 0)
		insertStatement(switchStmt,
				SchedCallBuilder::addToField(kernelFeatures, buildIntVal(scalarVals), MEMORY_RX));

	msg = "\tIdentifying number of work items...";
	DEBUG(TOOL, msg);
	SgFunctionSymbol* kernelSymbol =
			lookupFunctionSymbolInParentScopes(function->get_name() + "_x86", getScope(function));
	SgFunctionDeclaration* kernelDecl = kernelSymbol->get_declaration();
	ROSE_ASSERT(kernelDecl != NULL);

	//TODO assume for now only one OMP For section
	Rose_STL_Container<SgNode*> pragmas = querySubTree(kernelDecl, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt = pragmas.begin();
	SgForStatement* forStmt = NULL;
	SgPragmaDeclaration* pragma = NULL;
	SgExprStatement* assign = NULL;
	SgExprStatement* boundCheck = NULL;
	SgBinaryOp* assignExpr = NULL;
	SgBinaryOp* boundExpr = NULL;
	SgExpression* subExpr = NULL;
	for(; pragmaIt != pragmas.end(); pragmaIt++)
	{
		pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma);
		if(extractPragmaKeyword(pragma) == "omp"
				&& pragma->get_pragma()->get_pragma().find("for") != string::npos)
		{
			forStmt = isSgForStatement(getNextStatement(pragma));
			ROSE_ASSERT(forStmt != NULL);

			assign = isSgExprStatement(forStmt->get_for_init_stmt()->get_init_stmt()[0]);
			boundCheck = isSgExprStatement(forStmt->get_test());
			ROSE_ASSERT(assign != NULL && boundCheck != NULL);

			//TODO Assume single assign initializer & bounds check is single < or <=
			assignExpr = isSgBinaryOp(assign->get_expression());
			boundExpr = isSgBinaryOp(boundCheck->get_expression());
			ROSE_ASSERT(assignExpr != NULL && boundExpr != NULL);

			if(isSgIntVal(assignExpr->get_rhs_operand()) &&
					isSgIntVal(assignExpr->get_rhs_operand())->get_value() == 0)
				subExpr = boundExpr->get_rhs_operand();
			else
				subExpr = buildSubtractOp(boundExpr->get_rhs_operand(),
						assignExpr->get_rhs_operand());
			insertStatement(switchStmt,
					SchedCallBuilder::setField(kernelFeatures, subExpr, WORK_ITEMS));
		}
	}
}

/*
 * Adds in statements to sum variable sizes, and returns the number of scalar
 * variables found.
 */
int CallScheduler::sumVarSizes(set<string> vars, SgFunctionDefinition* funcDef, bool isInput)
{
	int scalarVals = 0;
	set<string>::const_iterator varIt = vars.begin();
	SgVariableSymbol* varSym = NULL;
	SgVarRefExp* varRef = NULL;
	SgType* type = NULL;
	SgType* baseType = NULL;
	int numDimensions = 0;
	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		string varName = *varIt;
		if(varName.find('*') != string::npos)
			varName = varName.substr(0, varName.length() - 1);

		varSym = lookupVariableSymbolInParentScopes(varName, funcDef);
		ROSE_ASSERT(varSym);

		type = varSym->get_type();
		baseType = NULL;
		numDimensions = 0;
		Misc::getType(&type, &baseType, &numDimensions);

		if(numDimensions > 1)
		{
			//Get inner & outer sizes & add to sum
			//Note: this assumes that data is matrix, i.e. uniform # of columns
			varRef = buildVarRefExp(varSym);
			//SgFunctionCallExp* outerSizeCall = MMCallBuilder::buildGetSizeCallExp(varRef, funcDef);
			SgPntrArrRefExp* varDeref = buildPntrArrRefExp(varRef, buildUnsignedLongVal(0));
			SgFunctionCallExp* innerSizeCall = MMCallBuilder::buildGetSizeCallExp(varDeref, funcDef);
			//SgMultiplyOp* sizeExp = buildMultiplyOp(outerSizeCall, innerSizeCall);
			if(isInput)
				insertStatement(switchStmt,
						SchedCallBuilder::addToField(kernelFeatures, innerSizeCall, MEMORY_TX));
			else
				insertStatement(switchStmt,
						SchedCallBuilder::addToField(kernelFeatures, innerSizeCall, MEMORY_RX));
		}
		else if(numDimensions == 1)
		{
			//Get size & add to sum
			varRef = buildVarRefExp(varSym);
			SgFunctionCallExp* sizeCall = MMCallBuilder::buildGetSizeCallExp(varRef, funcDef);
			if(isInput)
				insertStatement(switchStmt,
						SchedCallBuilder::addToField(kernelFeatures, sizeCall, MEMORY_TX));
			else
				insertStatement(switchStmt,
						SchedCallBuilder::addToField(kernelFeatures, sizeCall, MEMORY_RX));
		}
		else
			scalarVals++;
	}

	return scalarVals;
}



/*
 * Add a call to the runtime scheduler to select an implementation.
 */
void CallScheduler::addCallToScheduler()
{
	string msg = "\tAdding call to scheduler...";
	DEBUG(TOOL, msg);

	//Find partition_num variable
	Rose_STL_Container<SgNode*> vars = querySubTree(function, V_SgInitializedName);
	Rose_STL_Container<SgNode*>::const_iterator varIt = vars.begin();
	SgInitializedName* var = NULL;
	SgVariableDeclaration* varDecl = NULL;
	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		//TODO find a better way to grab this
		var = isSgInitializedName(*varIt);
		ROSE_ASSERT(var);

		if(NAME(var) == "__partition_num__")
			varDecl = isSgVariableDeclaration(var->get_declaration());
	}
	ROSE_ASSERT(varDecl != NULL);

	insertStatement(switchStmt, SchedCallBuilder::buildSchedulerCall(varDecl, kernelFeatures));
}

/*
 * Add a call to the runtime scheduler to cleanup after a kernel executes.
 */
void CallScheduler::addCallToCleanup()
{
	string msg = "\tAdding cleanup call after kernel execution";
	DEBUG(TOOL, msg);

	insertStatement(switchStmt, SchedCallBuilder::buildCleanupCall(kernelFeatures), false);
}
