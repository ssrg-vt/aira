/*
 * create_partition_mpi.cpp - partition a kernel onto an MPI accelerator.
 */

#include "rose.h" // Must come first for PCH to work.

#include "common.h"
#include "partition_kernels_common.h"
#include "create_partition.h"
#include "mpi_call_builder.h"
#include "miscellaneous.h"

/*
 * Static member definitions.
 */
bool MPIPartition::correctedHeader = false;

/*
 * Note: This will break if calling 'partition' from an arbitrary location.
 * This is okay for now, because it works with the driver script.
 */
const char* emptyMPITemplate = "./passes/partition_kernels/templates/empty_partition_mpi.c";
const char* correctedMPITemplate =
		"./passes/partition_kernels/templates/corrected_empty_partition_mpi.c";

/*
 * Partition constructor - Create an empty partition.
 */
MPIPartition::MPIPartition(const string& p_name, const string& outfileName) :
	Partition(p_name, emptyMPITemplate, outfileName),
	statusVar(NULL),
	statusVarDecl(NULL),
	functionSelect(NULL),
	datatypeDefFunc(NULL),
	datatypeDefBody(NULL)
{
  //Find status variable
  SgVariableSymbol* statusVarSymbol = lookupVariableSymbolInParentScopes(STATUSVAR_NAME,
		  getFirstGlobalScope(project));
  ROSE_ASSERT(statusVarSymbol);
  statusVar = statusVarSymbol->get_declaration();

  //Get status var declaration statement
  //TODO find a better way to insert functions
  statusVarDecl = isSgVariableDeclaration(statusVar->get_declaration());
  ROSE_ASSERT(statusVarDecl);

  //Find main switch statement
  SgFunctionDeclaration* main = findMain(project);
  ROSE_ASSERT(main != NULL);
  Rose_STL_Container<SgNode*> switchStmt = querySubTree(main, V_SgSwitchStatement);
  Rose_STL_Container<SgNode*>::const_iterator nodeIt = switchStmt.begin();
  for(nodeIt = switchStmt.begin(); nodeIt != switchStmt.end(); nodeIt++)
  {
	  if(isSgSwitchStatement(*nodeIt))
	  {
		  functionSelect = isSgSwitchStatement(*nodeIt);
		  break;
	  }
  }
  ROSE_ASSERT(functionSelect != NULL);
  ensureBasicBlockAsBodyOfSwitch(functionSelect);

  currentFuncNum = 0;
  string msg = "Created MPI partition " + *name;
  DEBUG(TOOL, msg);
}

/*
 * Unparse MPI partition AST to file and cleanup.
 */
MPIPartition::~MPIPartition()
{
  string msg = "Destroyed MPI partition " + *name;
  DEBUG(TOOL, msg);
}

/*
 * Return the partition type
 */
enum Hardware MPIPartition::getPartitionType()
{
	return MPI;
}

/*
 * moveFunction - create a new function in the partition and copy the body of
 * the passed function into the new function.
 */
Partition::FCode MPIPartition::moveFunction(SgFunctionDeclaration* func)
{
  ROSE_ASSERT(func != NULL);

  string msg = "Moving function " + NAME(func) + " to partition " + *name;
  DEBUG(TOOL, msg);

  SgFunctionDeclaration* newFunc = createEmptyFunction(func, false);
  copyFunctionBody(func, newFunc);

  currentFuncNum++;
  funcs.insert(pair<FCode, SgFunctionDeclaration*>(currentFuncNum, newFunc));
  insertCallToFunction(newFunc, currentFuncNum);

  return currentFuncNum;
}

/*
 * Move a supporting function (i.e. a function called by a kernel) into the
 * partition.
 */
Partition::FCode MPIPartition::moveSupportingFunction(SgFunctionDeclaration* func)
{
  ROSE_ASSERT(func != NULL);

  string msg = "Moving supporting function " + NAME(func) + " to partition " + *name;
  DEBUG(TOOL, msg);

  SgFunctionDeclaration* newFunc = createEmptyFunction(func, true);
  copyFunctionBody(func, newFunc);

  //Note: don't make this function externally available and don't insert a call
  //to it from the main MPI loop, as it should only be called from within a
  //kernel in a supporting fashion

  return currentFuncNum;
}

/*
 * createEmptyFunction - Insert an empty shell function into the template.
 */
SgFunctionDeclaration* MPIPartition::createEmptyFunction(SgFunctionDeclaration* func,
		bool isSupportingFunc)
{
  DEBUG(TOOL, "\tCreating function declaration...");

  SgTreeCopy deepCopy;
  string kernelName = NAME(func);
  if(kernelName.find("_x86") != string::npos)
	  kernelName = kernelName.substr(0, kernelName.length() - 4);
  SgName funcName(kernelName);
  if(!isSupportingFunc)
	  funcName = funcName + "_" + *name;

  //Get template file global scope
  SgGlobal* scope = templateFile->get_globalScope();
  ROSE_ASSERT(scope);
  ROSE_ASSERT(scope->supportsDefiningFunctionDeclaration());

  //Create empty function declaration
  SgFunctionDeclaration* newFunc = NULL;
  if(!isSupportingFunc)
	  newFunc = buildDefiningFunctionDeclaration(funcName, buildVoidType(),
			  buildFunctionParameterList(), scope);
  else
	  newFunc = buildDefiningFunctionDeclaration(funcName, func->get_orig_return_type(),
			  isSgFunctionParameterList(func->get_parameterList()->copy(deepCopy)), scope);

  //Insert into source
  insertStatement(statusVarDecl, newFunc, false);

  return newFunc;
}

/*
 * insertFunctionBody - Move the body of the function to be partitioned into
 * the shell function within the partition.
 */
void MPIPartition::copyFunctionBody(SgFunctionDeclaration* src, SgFunctionDeclaration* dest)
{
  DEBUG(TOOL, "\tCopying function body...");

  // Take the body of the old function and use that for the new function.
  SgTreeCopy deepCopy;
  SgFunctionDefinition *def = isSgFunctionDefinition(src->get_definition()->copy(deepCopy));
  ROSE_ASSERT(def != NULL);
  setSourcePositionForTransformation(def);

  // Link body and function together
  dest->set_definition(def);
  def->set_declaration(dest);
}

/*
 * insertCallToFunction - add a given function to the 'while (!finished) { ...
 * }' MPI loop.
 */
void MPIPartition::insertCallToFunction(SgFunctionDeclaration *func, FCode funcNum)
{
  DEBUG(TOOL, "\tInserting call to function in main loop...");

  //Add a call to the function
  SgFunctionSymbol* funcSymbol = isSgFunctionSymbol(func->search_for_symbol_from_symbol_table());
  ROSE_ASSERT(funcSymbol);
  SgFunctionCallExp* funcCallExp = buildFunctionCallExp(funcSymbol);
  SgExprStatement* funcCall = buildExprStatement(funcCallExp);
  SgBasicBlock* caseBody = buildBasicBlock(funcCall, buildBreakStmt());
  SgCaseOptionStmt* caseStmt = buildCaseOptionStmt(buildIntVal(funcNum), caseBody);
  SgBasicBlock* switchBody = isSgBasicBlock(functionSelect->get_body());
  ROSE_ASSERT(switchBody);
  insertStatement(getLastStatement(switchBody), caseStmt);
}

/*
 * Perform all necessary final steps for partitioning the function.
 */
void MPIPartition::finalizeFunction(FCode funcNum)
{
	SgFunctionDeclaration* func = funcs.find(funcNum)->second;
	ROSE_ASSERT(func);

	//Add inputs
	insertStatementList(getFirstStatement(func->get_definition()), inputStmts);

	//Add outputs (w/ iterator because no way to instrument end of function w/ statement lists)
	vector<SgStatement*>::const_iterator stmtIt;
	for(stmtIt = outputStmts.begin(); stmtIt != outputStmts.end(); stmtIt++)
		  instrumentEndOfFunction(func, *stmtIt);

	//Free any remaining vars
	map<string, SgInitializedName*>::const_iterator varIt = mallocedVars.begin();
	for(varIt = mallocedVars.begin(); varIt != mallocedVars.end(); varIt++)
		freeVar(varIt->second, func);

	//Prepare for next function
	inputStmts.clear();
	outputStmts.clear();
	mallocedVars.clear();
}

/*
 * Internal helper method that inserts the necessary calls to free a variable.
 */
void MPIPartition::freeVar(SgInitializedName* buffer, SgFunctionDeclaration* func)
{
	SgFunctionDefinition* funcDef = func->get_definition();
	SgType* type = buffer->get_type();
	SgType* baseType = NULL;
	int numDimensions = 0;
	Misc::getType(&type, &baseType, &numDimensions);

	string msg = "\tAdding calls to free variable " + NAME(buffer);
	DEBUG(TOOL, msg);

	if(numDimensions > 1)
	{
		//Get ref to hidden array
		SgVarRefExp* hiddenBuf = buildVarRefExp(NAME(buffer) + HIDDEN_NAME, funcDef);

		//Add loop + free outer pointer
		instrumentEndOfFunction(func, buildFreeCall(hiddenBuf, funcDef));
		instrumentEndOfFunction(func, buildFreeCall(buffer, funcDef));
	}
	else
	{
		//Should not be freeing variables that didn't require a malloc
		ROSE_ASSERT(numDimensions == 1);

		//Free data
		instrumentEndOfFunction(func, buildFreeCall(buffer, funcDef));
	}
}

/*
 * addInput - Receive input via MPI_Receive on MPI partition.
 */
void MPIPartition::addInput(Partition::FCode funcNum, SgInitializedName* buffer)
{
  SgFunctionDeclaration* func = funcs.find(funcNum)->second;
  ROSE_ASSERT(func);
  ROSE_ASSERT(buffer);
  SgFunctionDefinition* funcDef = func->get_definition();

  //If var is constant, copy definition over to partition (no need to transfer)
  SgType* type = buffer->get_type();
  if(isConstType(type))
  {
	  SgTreeCopy deepCopy;
	  SgDeclarationStatement* toCopy = buffer->get_declaration()->get_definingDeclaration();
	  ROSE_ASSERT(toCopy);
	  SgDeclarationStatement* constVar = isSgDeclarationStatement(toCopy->copy(deepCopy));
	  ROSE_ASSERT(constVar);
	  setSourcePositionForTransformation(constVar);
	  inputStmts.push_back(constVar);
	  return;
  }

  //Dig through typedef/pointer/array types to get base type
  SgType* baseType = NULL;
  int numDimensions = 0;
  Misc::getType(&type, &baseType, &numDimensions);

  //Declare variable(s)
  if(baseType)
  {
	  if(numDimensions > 1)
	  {
		  //TODO for now only accept double pointers
		  ROSE_ASSERT(numDimensions == 2);

		  //Declare size variables + loop iterator
		  SgName outerSizeName(NAME(buffer) + OUTERSIZE_NAME);
		  SgName innerSizeName(NAME(buffer) + INNERSIZE_NAME);
		  SgName loopItName(NAME(buffer) + LOOPIT_NAME);
		  SgVariableDeclaration* outerSizeDecl =
				  buildVariableDeclaration(outerSizeName, buildUnsignedLongType(), NULL, funcDef);
		  SgVariableDeclaration* innerSizeDecl =
				  buildVariableDeclaration(innerSizeName, buildUnsignedLongType(), NULL, funcDef);
		  SgVariableDeclaration* loopItDecl =
				  buildVariableDeclaration(loopItName, buildUnsignedLongType(), NULL, funcDef);
		  inputStmts.push_back(outerSizeDecl);
		  inputStmts.push_back(innerSizeDecl);
		  inputStmts.push_back(loopItDecl);

		  //Declare outer buffer + hidden inner buffer
		  int i = 0;
		  type = baseType;
		  for(i = 0; i < numDimensions; i++)
			  type = buildPointerType(type);
		  SgVariableDeclaration* recvBuffer =
				  buildVariableDeclaration(NAME(buffer), type, NULL, funcDef);
		  type = baseType; //TODO needs to be adjusted for > 2 dimensions
		  for(i = 0; i < numDimensions - 1; i++)
			  type = buildPointerType(type);
		  SgVariableDeclaration* hiddenBuffer =
				  buildVariableDeclaration(NAME(buffer) + HIDDEN_NAME, type, NULL, funcDef);
		  inputStmts.push_back(recvBuffer);
		  inputStmts.push_back(hiddenBuffer);
		  type = buildPointerType(type);
	  }
	  else
	  {
		  //Declare size variable
		  SgName sizeName(NAME(buffer) + SIZE_NAME);
		  SgVariableDeclaration* sizeDecl =
				  buildVariableDeclaration(sizeName, buildUnsignedLongType(), NULL, funcDef);
		  inputStmts.push_back(sizeDecl);

		  //Declare receive buffer
		  type = buildPointerType(baseType);
		  SgVariableDeclaration* recvBuffer =
				  buildVariableDeclaration(NAME(buffer), type, NULL, funcDef);
		  inputStmts.push_back(recvBuffer);
	  }
  }
  else
  {
	  //Declare receive buffer
	  SgVariableDeclaration* recvBuffer =
			  buildVariableDeclaration(NAME(buffer), type, NULL, funcDef);
	  inputStmts.push_back(recvBuffer);
  }

  //Receive data
  SgInitializedName* bufVar = buildInitializedName(NAME(buffer), type);
  addInputInternal(funcDef, bufVar);
}

/*
 * Add a global input to the function
 */
void MPIPartition::addGlobalInput(FCode funcNum, SgInitializedName* buffer)
{
	SgFunctionDeclaration* func = funcs.find(funcNum)->second;
	SgVariableDeclaration* globalVar = NULL;
	ROSE_ASSERT(func);
	ROSE_ASSERT(buffer);

	//Dig through typedef/pointer/array types to get base type
	SgInitializedName* bufVar = NULL;
	SgType* type = buffer->get_type();
	SgType* baseType = NULL;
	int numDimensions = 0;
	Misc::getType(&type, &baseType, &numDimensions);

	//Declare variable, if not already declared
	if(globalVars.find(NAME(buffer)) == globalVars.end())
	{
		//If var is constant, copy definition over to partition (no need to receive)
		if(isConstType(type))
		{
			SgTreeCopy deepCopy;
			SgDeclarationStatement* toCopy = buffer->get_declaration();
			SgVariableDeclaration* constVar = isSgVariableDeclaration(toCopy->copy(deepCopy));
			ROSE_ASSERT(constVar);
			setSourcePositionForTransformation(constVar);
			globalVars.insert(pair<string, SgVariableDeclaration*>(NAME(buffer), constVar));
			insertStatement(statusVarDecl, constVar);
			return;
		}

		//Convert array types to pointers
		type = (baseType != NULL ? baseType : type);
		for(int i = 0; i < numDimensions; i++)
			type = buildPointerType(type);
		globalVar = buildVariableDeclaration(NAME(buffer), type, NULL, func->get_scope());
		globalVars.insert(pair<string, SgVariableDeclaration*>(NAME(buffer), globalVar));
		insertStatement(statusVarDecl, globalVar);

		//Declare size variable(s) & iterators
		if(numDimensions > 1)
		{
			//TODO for now, only accept double pointers
			ROSE_ASSERT(numDimensions == 2);

			SgName outerSizeName(NAME(buffer) + OUTERSIZE_NAME);
			SgName innerSizeName(NAME(buffer) + INNERSIZE_NAME);
			SgName loopItName(NAME(buffer) + LOOPIT_NAME);
			SgVariableDeclaration* outerSizeDecl = buildVariableDeclaration(outerSizeName,
					buildUnsignedLongType(), NULL, getFirstGlobalScope(project));
			SgVariableDeclaration* innerSizeDecl = buildVariableDeclaration(innerSizeName,
					buildUnsignedLongType(), NULL, getFirstGlobalScope(project));
			SgVariableDeclaration* loopItDecl = buildVariableDeclaration(loopItName,
					buildUnsignedLongType(), NULL, getFirstGlobalScope(project));
			insertStatement(globalVar, outerSizeDecl);
			insertStatement(globalVar, innerSizeDecl);
			insertStatement(globalVar, loopItDecl);

			//Declare inner hidden buffer
			SgName hiddenBufName(NAME(buffer) + HIDDEN_NAME);
			SgType* hiddenType = (baseType != NULL ? baseType : type);
			for(int i = 0; i < numDimensions - 1; i++)
				hiddenType = buildPointerType(hiddenType);
			SgVariableDeclaration* hiddenBufDecl = buildVariableDeclaration(hiddenBufName,
					hiddenType, NULL, getFirstGlobalScope(project));
			insertStatement(globalVar, hiddenBufDecl);
		}
		else if(numDimensions == 1)
		{
			SgName sizeName(NAME(buffer) + SIZE_NAME);
			SgVariableDeclaration* sizeVar = buildVariableDeclaration(sizeName,
					buildUnsignedLongType(), NULL, getFirstGlobalScope(project));
			insertStatement(globalVar, sizeVar);
		}
	}

	//Add receive
	bufVar = buildInitializedName(NAME(buffer), type);
	addInputInternal(func->get_definition(), bufVar);
}

/*
 * Add call(s) to receive an input via MPI in the specified function.
 */
void MPIPartition::addInputInternal(SgFunctionDefinition* funcDef, SgInitializedName* buffer)
{
	SgType* type = buffer->get_type();
	SgType* baseType = NULL;
	int numDimensions = 0;
	Misc::getType(&type, &baseType, &numDimensions);

	SgExprStatement* receive = NULL;
	if(baseType)
	{
		//Save buffer as needing to be freed
		mallocedVars.insert(pair<string, SgInitializedName*>(NAME(buffer), buffer));

		if(numDimensions > 1)
		{
			//TODO only accept double pointers
			ROSE_ASSERT(numDimensions == 2);

			//Get size + loop iterator declarations + hidden array
			SgInitializedName* outerSizeVar =
					buildInitializedName(NAME(buffer) + OUTERSIZE_NAME, buildUnsignedLongType());
			SgInitializedName* innerSizeVar =
					buildInitializedName(NAME(buffer) + INNERSIZE_NAME, buildUnsignedLongType());
			SgInitializedName* loopItVar =
					buildInitializedName(NAME(buffer) + LOOPIT_NAME, buildUnsignedLongType());
			SgType* hiddenType = baseType;
			for(int i = 0; i < numDimensions - 1; i++)
				hiddenType = buildPointerType(hiddenType);

			//Receive outer size from host
			receive = MPICallBuilder::buildMPIReceive(outerSizeVar, funcDef, X86, statusVar);
			inputStmts.push_back(receive);

			//Receive inner size from host
			receive = MPICallBuilder::buildMPIReceive(innerSizeVar, funcDef, X86, statusVar);
			inputStmts.push_back(receive);

			//Initialize outer pointer w/ call to malloc
			SgCastExp* cast = buildMallocCall(NAME(outerSizeVar), buildPointerType(baseType), funcDef);
			SgAssignOp* bufAssign = buildAssignOp(buildVarRefExp(NAME(buffer), funcDef), cast);
			SgExprStatement* assignStmt = buildExprStatement(bufAssign);
			inputStmts.push_back(assignStmt);

			//Initialize inner pointer w/ call to malloc
			SgMultiplyOp* totalSize = buildMultiplyOp(buildVarRefExp(outerSizeVar, funcDef),
					buildVarRefExp(innerSizeVar, funcDef));
			cast = buildMallocCall(totalSize, baseType, funcDef);
			bufAssign = buildAssignOp(buildVarRefExp(NAME(buffer) + HIDDEN_NAME, funcDef), cast);
			assignStmt = buildExprStatement(bufAssign);
			inputStmts.push_back(assignStmt);

			//Build for-loop
			SgVarRefExp* loopVarRef = buildVarRefExp(loopItVar);
			SgExprStatement* init =
					buildAssignStatement(loopVarRef, buildUnsignedLongVal(0));
			SgExprStatement* check = buildExprStatement(
					buildLessThanOp(loopVarRef, buildVarRefExp(outerSizeVar)));
			SgExpression* update = buildPlusPlusOp(loopVarRef);

			//For now, we assume that all inner sizes are the same
			SgBasicBlock* body = buildBasicBlock();
			SgPntrArrRefExp* arrRef = buildPntrArrRefExp(buildVarRefExp(buffer, funcDef),
					loopVarRef);

			//Set address in outer array to location in inner array
			SgMultiplyOp* innerElem = buildMultiplyOp(buildVarRefExp(innerSizeVar, funcDef),
					buildVarRefExp(loopItVar, funcDef));
			SgPntrArrRefExp* innerArrRef = buildPntrArrRefExp(
					buildVarRefExp(NAME(buffer) + HIDDEN_NAME, funcDef), innerElem);
			SgAddressOfOp* addrOfInner = buildAddressOfOp(innerArrRef);
			body->append_statement(buildAssignStatement(arrRef, addrOfInner));

			///////////////////////////////////////////////////////////////////
			// This code receives a double-pointer by receiving each internal
			// pointer individually, assuming a non-contiguous host buffer
			/*body->append_statement(buildAssignStatement(arrRef,
					buildMallocCall(NAME(innerSizeVar), baseType, funcDef)));

			if(isSgClassType(baseType))
				addClassMPITransferCalls(body->get_statements(), funcDef, true, buffer,
						type, baseType);
			else
				body->append_statement(MPICallBuilder::buildMPIReceive(arrRef, funcDef, X86,
						statusVar, buildVarRefExp(innerSizeVar)));*/
			//
			///////////////////////////////////////////////////////////////////

			inputStmts.push_back(buildForStatement(init, check, update, body));

			//Receive data
			innerArrRef = buildPntrArrRefExp(
					buildVarRefExp(NAME(buffer) + HIDDEN_NAME, funcDef), buildUnsignedLongVal(0));
			if(isSgClassType(baseType))
				addClassMPITransferCalls(inputStmts, funcDef, true, buildVarRefExp(buffer),
						isSgClassType(type), totalSize);
			else
				inputStmts.push_back(MPICallBuilder::buildMPIReceive(
						buildVarRefExp(NAME(buffer) + HIDDEN_NAME, funcDef), funcDef, X86,
						statusVar, totalSize));
		}
		else
		{
			//Get size declaration
			SgInitializedName* sizeVar =
					buildInitializedName(NAME(buffer) + SIZE_NAME, buildUnsignedLongType());

			//Receive size from host
			receive = MPICallBuilder::buildMPIReceive(sizeVar, funcDef, X86, statusVar);
			inputStmts.push_back(receive);

			//Initialize receive buffer w/ call to malloc
			SgCastExp* cast = buildMallocCall(NAME(sizeVar), baseType, funcDef);
			SgAssignOp* bufAssign = buildAssignOp(buildVarRefExp(NAME(buffer), funcDef), cast);
			SgExprStatement* assignStmt = buildExprStatement(bufAssign);
			inputStmts.push_back(assignStmt);

			//Receive data
			if(isSgClassType(baseType))
				addClassMPITransferCalls(inputStmts, funcDef, true, buffer, isSgClassType(baseType),
						sizeVar);
			else
			{
				receive = MPICallBuilder::buildMPIReceive(buffer, funcDef, X86, statusVar, sizeVar);
				inputStmts.push_back(receive);
			}
		}
	}
	else if(isSgClassType(type))
		addClassMPITransferCalls(inputStmts, funcDef, true, buffer, isSgClassType(type),
				NULL); //Super-duper ugly, but otherwise compiler complains
	else
	{
		receive = MPICallBuilder::buildMPIReceive(buildInitializedName(NAME(buffer),
				buffer->get_type()), funcDef, X86, statusVar);
		inputStmts.push_back(receive);
	}
}

/*
 * addOutput - Transfer back output from MPI partition via MPI_Send
 */
void MPIPartition::addOutput(Partition::FCode funcNum, SgInitializedName* buffer)
{
  SgFunctionDeclaration* func = funcs.find(funcNum)->second;
  ROSE_ASSERT(func);
  ROSE_ASSERT(buffer);

  //TODO do we need to declare variables here?

  addOutputInternal(func->get_definition(), buffer);
}

/*
 * Add a global output to the function
 */
void MPIPartition::addGlobalOutput(FCode funcNum, SgInitializedName* buffer)
{
	SgFunctionDeclaration* func = funcs.find(funcNum)->second;
	ROSE_ASSERT(func);
	ROSE_ASSERT(buffer);

	//TODO do we need to declare variables here?

	addOutputInternal(func->get_definition(), buffer);
}

/*
 * Internal function.
 *
 * Add a call to send an output via MPI in the specified function.
 */
void MPIPartition::addOutputInternal(SgFunctionDefinition* funcDef, SgInitializedName* buffer)
{
	SgStatement* stmt = NULL;
	SgType* type = buffer->get_type();
	SgType* baseType = NULL;
	int numDimensions = 0;
	Misc::getType(&type, &baseType, &numDimensions);

	if(baseType)
	{
		//Mark this var as freed
		mallocedVars.erase(mallocedVars.find(NAME(buffer)));

		if(numDimensions > 1)
		{
			//TODO only handle double pointers right now
			ROSE_ASSERT(numDimensions == 2);

			//Get references to size + loop iterator + hidden buf variables
			//SgVarRefExp* bufRef = buildVarRefExp(NAME(buffer), funcDef);
			SgVarRefExp* outerSizeRef = buildVarRefExp(NAME(buffer) + OUTERSIZE_NAME, funcDef);
			SgVarRefExp* innerSizeRef = buildVarRefExp(NAME(buffer) + INNERSIZE_NAME, funcDef);
			//SgVarRefExp* itRef = buildVarRefExp(NAME(buffer) + LOOPIT_NAME, funcDef);
			SgVarRefExp* hiddenBufRef = buildVarRefExp(NAME(buffer) + HIDDEN_NAME, funcDef);

			///////////////////////////////////////////////////////////////////
			// This code frees sends back/frees each internal pointer
			// individually

			//Build for loop
			/*SgExprStatement* init = buildAssignStatement(itRef, buildUnsignedLongVal(0));
			SgExprStatement* test = buildExprStatement(buildLessThanOp(itRef, outerSizeRef));
			SgExpression* update = buildPlusPlusOp(itRef);

			SgBasicBlock* body = buildBasicBlock();
			SgPntrArrRefExp* arrRef = buildPntrArrRefExp(bufRef, itRef);

			//Transfer data back
			if(isSgClassType(baseType))
				addClassMPITransferCalls(outputStmts, funcDef, false, buffer, type, baseType);
			else
				body->append_statement(MPICallBuilder::buildMPISend(arrRef, funcDef,
						X86, innerSizeRef));
			body->append_statement(buildFreeCall(arrRef, funcDef));
			outputStmts.push_back(buildForStatement(init, test, update, body));*/
			//
			///////////////////////////////////////////////////////////////////

			//Transfer data back & free buffers
			SgMultiplyOp* totalSize = buildMultiplyOp(outerSizeRef, innerSizeRef);
			if(isSgClassType(baseType))
				addClassMPITransferCalls(outputStmts, funcDef, false, buildVarRefExp(buffer),
						isSgClassType(baseType), totalSize);
			else
				outputStmts.push_back(MPICallBuilder::buildMPISend(hiddenBufRef, funcDef, X86,
						totalSize));
			outputStmts.push_back(buildFreeCall(hiddenBufRef, funcDef));
			outputStmts.push_back(buildFreeCall(buffer, funcDef));
		}
		else
		{
			//Get size variables
			SgInitializedName* sizeVar =
					buildInitializedName(NAME(buffer) + SIZE_NAME, buildUnsignedLongType());

			//Transfer data back
			if(isSgClassType(baseType))
				addClassMPITransferCalls(outputStmts, funcDef, false, buffer, isSgClassType(baseType),
						sizeVar);
			else
			{
				stmt = MPICallBuilder::buildMPISend(buffer, funcDef, X86, sizeVar);
				outputStmts.push_back(stmt);
			}

			//Free data
			stmt = buildFreeCall(buffer, funcDef);
			outputStmts.push_back(stmt);
		}
	}
	else if(isSgClassType(type))
		addClassMPITransferCalls(outputStmts, funcDef, false, buffer, isSgClassType(type),
				NULL); //Super-duper ugly, but otherwise compiler complains
	else
	{
		stmt = MPICallBuilder::buildMPISend(buffer, funcDef, X86);
		outputStmts.push_back(stmt);
	}
}

/*
 * Add calls necessary to transfer a struct via MPI.
 *
 * Specify variable + size with an initialized name
 */
void MPIPartition::addClassMPITransferCalls(vector<SgStatement*>& stmts,
		SgFunctionDefinition* funcDef, bool isInput, SgInitializedName* var,
		SgClassType* type, SgInitializedName* sizeVar)
{
	if(sizeVar)
		addClassMPITransferCalls(stmts, funcDef, isInput, buildVarRefExp(var), type,
				buildVarRefExp(sizeVar));
	else
		addClassMPITransferCalls(stmts, funcDef, isInput, buildVarRefExp(var), type,
				NULL);
}

/*
 * Add calls necessary to transfer a struct via MPI.
 *
 * This version allows an expression to be supplied as the LHS of an assignment.
 */
void MPIPartition::addClassMPITransferCalls(vector<SgStatement*>& stmts,
		SgFunctionDefinition* funcDef, bool isInput, SgExpression* var, SgClassType* type,
		SgExpression* sizeVar)
{
	string msg;

	SgVarRefExp* datatypeVar = NULL;
	bool containsPointers = Misc::classContainsPointers(type);
	string name = NAME(type) + "__datatype";

	if(isInput)
	{
		if(!containsPointers)
		{
			map<string, SgVariableDeclaration*>::const_iterator varIt =
					mpiDatatypes.find(NAME(type) + "__datatype");
			ROSE_ASSERT(varIt != mpiDatatypes.end());
			datatypeVar = buildVarRefExp(varIt->second);

			//Receive struct
			if(sizeVar)
				stmts.push_back(MPICallBuilder::buildMPIReceive(var, funcDef, X86, statusVar,
						datatypeVar, sizeVar));
			else
				stmts.push_back(MPICallBuilder::buildMPIReceive(var, funcDef, X86, statusVar,
						datatypeVar));
		}
		else
		{
			//TODO handle structs w/ pointers
		}
	}
	else
	{
		if(!containsPointers)
		{
			map<string, SgVariableDeclaration*>::const_iterator varIt =
					mpiDatatypes.find(NAME(type) + "__datatype");
			ROSE_ASSERT(varIt != mpiDatatypes.end());
			datatypeVar = buildVarRefExp(varIt->second);

			//Send struct
			if(sizeVar)
				stmts.push_back(MPICallBuilder::buildMPISend(var, funcDef, X86, datatypeVar, sizeVar));
			else
				stmts.push_back(MPICallBuilder::buildMPISend(var, funcDef, X86, datatypeVar));
		}
		else
		{
			//TODO handle structs w/ pointers
		}
	}
}

/*
 * Add a class (including an MPI defined datatype) to the MPI partition.
 */
void MPIPartition::addClassDeclaration(SgClassType* type)
{
	string msg;

	//Check to see if class has already been inserted
	SgName datatypeName(NAME(type) + "__datatype");
	if(mpiDatatypes.find(datatypeName) != mpiDatatypes.end())
		return;

	//Copy class declaration into partition
	SgTreeCopy deepCopy;
	SgClassDeclaration* classDecl =
			isSgClassDeclaration(type->get_declaration()->get_definingDeclaration()->copy(deepCopy));
	insertStatement(statusVarDecl, classDecl);
	setSourcePositionForTransformation(classDecl);

	//Define a function to declare all types
	if(!datatypeDefFunc)
	{
		DEBUG(TOOL, "Creating a function to define all MPI datatypes (partition)");

		datatypeDefFunc = buildDefiningFunctionDeclaration("__defineMPIDatatypes", buildVoidType(),
				buildFunctionParameterList(), getFirstGlobalScope(project));
		datatypeDefBody = datatypeDefFunc->get_definition();

		SgFunctionDeclaration* main = findMain(project);
		insertStatement(main, datatypeDefFunc);
		SgExprStatement* funcCall = buildFunctionCallStmt(NAME(datatypeDefFunc),
				buildVoidType(), NULL, main->get_definition());

		//Slightly hacky way to get MPI_Init statement, but should be constant
		SgStatement* mpiInit = getFirstStatement(main->get_definition());
		mpiInit = getNextStatement(mpiInit);
		insertStatement(mpiInit, funcCall, false);
	}

	//Add statements to declare datatype
	vector<SgStatement*> stmts;
	if(!Misc::classContainsPointers(type))
	{
		//Class doesn't contain pointers, can define datatype to send
		msg = "\tDefining MPI datatype for " + NAME(type) + " (partition)";
		DEBUG(TOOL, msg);

		//Declare datatype var
		SgVariableDeclaration* datatype = buildVariableDeclaration(datatypeName,
				buildOpaqueType("MPI_Datatype", classDecl->get_scope()), NULL,
				classDecl->get_scope());
		insertStatement(classDecl, datatype, false);
		mpiDatatypes.insert(pair<string, SgVariableDeclaration*>(datatypeName, datatype));

		//Create code to initialize & commit new datatype
		vector<SgStatement*> datatypeDecl =
				MPICallBuilder::buildMPIDatatypeDecl(type, datatypeDefBody, datatype);
		stmts.insert(stmts.end(), datatypeDecl.begin(), datatypeDecl.end());
	}
	else
	{
		//TODO handle structs w/ pointers
		//Class contains pointers, must send individual members
		DEBUG(TOOL, "Cannot send structs/classes containing pointers at this time!");
		ROSE_ASSERT(false);
	}

	for(unsigned int i = 0; i < stmts.size(); i++)
		datatypeDefBody->append_statement(stmts[i]);
}

/*
 * Internal method.
 *
 * Build a call to malloc.
 */
SgCastExp* MPIPartition::buildMallocCall(SgName bufSizeVar, SgType* bufElemType,
		SgScopeStatement* scope)
{
	SgBinaryOp* mallocSize = buildMultiplyOp(buildSizeOfOp(bufElemType), buildVarRefExp(bufSizeVar));
	SgFunctionCallExp* mallocCall = buildFunctionCallExp("malloc", buildPointerType(buildVoidType()),
			buildExprListExp(mallocSize), scope);
	return buildCastExp(mallocCall, buildPointerType(bufElemType));
}

/*
 * Internal method.
 *
 * Build a call to malloc.  This version of the overloaded function allows you
 * to give an expression representing the number of elements to malloc.
 */
SgCastExp* MPIPartition::buildMallocCall(SgExpression* bufExp, SgType* bufElemType,
		SgScopeStatement* scope)
{
	SgBinaryOp* mallocSize = buildMultiplyOp(buildSizeOfOp(bufElemType), bufExp);
	SgFunctionCallExp* mallocCall = buildFunctionCallExp("malloc", buildPointerType(buildVoidType()),
			buildExprListExp(mallocSize), scope);
	return buildCastExp(mallocCall, buildPointerType(bufElemType));
}

/*
 * Internal method.
 *
 * Build a call to free, with the specified initialized name as the argument.
 */
SgExprStatement* MPIPartition::buildFreeCall(SgInitializedName* buffer, SgScopeStatement* scope)
{
	SgVarRefExp* bufRef = buildVarRefExp(NAME(buffer), scope);
	return buildFreeCall(bufRef, scope);
}

/*
 * Internal method.
 *
 * Build a call to free, with the specified expression as an argument.
 */
SgExprStatement* MPIPartition::buildFreeCall(SgExpression* var, SgScopeStatement* scope)
{
	SgExprListExp* args = buildExprListExp(var);
	SgFunctionCallExp* freeCall = buildFunctionCallExp("free", buildVoidType(), args, scope);
	SgExprStatement* call = buildExprStatement(freeCall);
	return call;
}

/*
 * setMPIHeader - set the MPI header location for the MPI template (allows the
 * user to specify a custom location or defaults to mpi.h)
 */
void MPIPartition::setMPIHeader(const string& mpiHeader)
{
	string comment = "// @@MPI_INCLUDE_HERE@@";
	if(!correctedHeader)
	{
		setHeader(mpiHeader, comment, emptyMPITemplate, correctedMPITemplate);
		emptyMPITemplate = correctedMPITemplate;
		correctedHeader = true;
	}
	else
		setHeader(mpiHeader, comment, emptyMPITemplate, correctedMPITemplate);
}

/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
