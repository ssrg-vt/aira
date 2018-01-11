/*
 * partition_kernel.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "partition_kernels_common.h"
#include "pragma_handling.h"
#include "partition_kernel.h"
#include "mpi_call_builder.h"
#include "mm_call_builder.h"
#include "miscellaneous.h"

/*
 * Define static class members.
 */
map<string, SgVariableDeclaration*> PartitionKernel::mpiDatatypes;
SgFunctionDeclaration* PartitionKernel::datatypeDefFunc;
SgFunctionDefinition* PartitionKernel::datatypeDefBody;

/*
 * Default constructor.
 */
PartitionKernel::PartitionKernel(SgFunctionDeclaration* p_kernel, Pragmas& p_pragmas) :
	kernel(p_kernel),
	stub(NULL),
	gpuPrototype(NULL),
	mpiStub(NULL),
	switchStmt(NULL),
	partitionVar(NULL),
	pragmas(p_pragmas)
{
	string msg;
	ROSE_ASSERT(kernel != NULL);

	//Create kernel launch stub
	SgName name(NAME(kernel));
	SgTreeCopy deepCopy;
	SgFunctionParameterList* paramList =
			isSgFunctionParameterList(kernel->get_parameterList()->copy(deepCopy));
	ROSE_ASSERT(paramList != NULL);
	SgExprListExp* decList = NULL;
	if(kernel->get_decoratorList())
	{
		decList = isSgExprListExp(kernel->get_decoratorList()->copy(deepCopy));
		ROSE_ASSERT(decList != NULL);
	}
	SgFunctionDeclaration* origKernel = buildDefiningFunctionDeclaration(name,
			kernel->get_orig_return_type(), paramList, kernel->get_scope(), decList);
	origKernel->get_definition()->set_body(kernel->get_definition()->get_body());
	origKernel->get_definition()->get_body()->set_parent(origKernel->get_definition());
	kernel->get_definition()->set_body(buildBasicBlock());
	kernel->get_definition()->get_body()->set_parent(kernel);
	stub = kernel;
	kernel = origKernel;
	name += "_x86";
	kernel->set_name(name);
	insertStatement(pragmas.getFirstPragma(), origKernel);

	msg = "Created stub for " + NAME(stub) + " and renamed original kernel " + NAME(kernel);
	DEBUG(TOOL, msg);

	//Add initial code to call original kernel
	SgName partVarName("__partition_num__");
	SgInitializer* partNumInit = buildAssignInitializer(buildUnsignedLongVal(X86));
	partitionVar = buildVariableDeclaration(partVarName, buildUnsignedLongType(),
			partNumInit, stub->get_definition());
	stub->get_definition()->append_statement(partitionVar);
	switchStmt = buildSwitchStatement(buildVarRefExp(partVarName, stub->get_scope()),
			buildBasicBlock());
	stub->get_definition()->append_statement(switchStmt);

	//Add call to x86 as default
	vector<SgExpression*> params;
	SgInitializedNamePtrList& kernelParams = stub->get_parameterList()->get_args();
	SgInitializedNamePtrList::const_iterator paramIt = kernelParams.begin();
	for(paramIt = kernelParams.begin(); paramIt != kernelParams.end(); paramIt++)
		params.push_back(buildVarRefExp(*paramIt, stub->get_definition()));
	SgExprListExp* callParams = buildExprListExp(params);
	SgDefaultOptionStmt* x86Kernel = buildDefaultCallCase(kernel, callParams);
	switchStmt->append_default(x86Kernel);

	partitions.insert(X86_STR);
	DEBUG(TOOL, "\tInitialized for implementation selection");
}

/*
 * Partition a kernel to a GPU.
 */
void PartitionKernel::partitionToGPU(GPUPartition* partition)
{
	Partition::FCode funcCode = partition->moveFunction(kernel);
	copySupportingFunctions(partition);

	//Move required class declarations to GPU
	set<string> classes = pragmas.getClassesNeeded()->getNames();
	set<string>::const_iterator classIt = classes.begin();
	SgClassSymbol* classSym = NULL;
	for(classIt = classes.begin(); classIt != classes.end(); classIt++)
	{
		classSym = lookupClassSymbolInParentScopes(*classIt, getScope(kernel));
		ROSE_ASSERT(classSym != NULL);
		SgClassType* classType = isSgClassType(classSym->get_type());
		ROSE_ASSERT(classType != NULL);
		partition->addClassDeclaration(classType);
	}

	//Move global variables to GPU
	//TODO move global outputs as well?
	set<string> globalVars = pragmas.getGlobalInputs()->getNames();
	set<string>::const_iterator varIt = globalVars.begin();
	SgVariableSymbol* varSymbol = NULL;
	for(varIt = globalVars.begin(); varIt != globalVars.end(); varIt++)
	{
		string name(*varIt);
		if(name.rfind('*') != string::npos)
			name.resize(name.length() - 1);
		varSymbol = lookupVariableSymbolInParentScopes(name, getScope(kernel));
		ROSE_ASSERT(varSymbol);

		partition->addGlobalVar(varSymbol->get_declaration());
	}

	//Insert prototype to GPU function
	SgTreeCopy deepCopy;
	string name = partition->getFuncName(funcCode);
	ROSE_ASSERT(name != "");
	SgFunctionParameterList* gpuParamList =
			isSgFunctionParameterList(kernel->get_parameterList()->copy(deepCopy));
	gpuPrototype = buildNondefiningFunctionDeclaration(name, kernel->get_orig_return_type(),
			gpuParamList, stub->get_scope(), stub->get_decoratorList());
	insertStatement(pragmas.getFirstPragma(), gpuPrototype);

	//Add switch case to stub
	vector<SgExpression*> params;
	SgInitializedNamePtrList& kernelParams = stub->get_parameterList()->get_args();
	SgInitializedNamePtrList::const_iterator paramIt = kernelParams.begin();
	for(paramIt = kernelParams.begin(); paramIt != kernelParams.end(); paramIt++)
		params.push_back(buildVarRefExp(*paramIt, stub->get_definition()));
	SgExprListExp* gpuParams = buildExprListExp(params);
	SgCaseOptionStmt* gpuCase = buildCallCase(gpuPrototype, gpuParams, buildUnsignedLongVal(GPU));
	switchStmt->append_case(gpuCase);

	partitions.insert(GPU_STR);
	string msg = "Added GPU implementation for " + stub->get_name();
	DEBUG(TOOL, msg);
}

/*
 * Partition a kernel to an architecture via MPI.
 */
void PartitionKernel::partitionToMPI(MPIPartition* partition)
{
	Partition::FCode funcCode = partition->moveFunction(kernel);
	copySupportingFunctions(partition);

	//Move required class declarations to MPI partition
	set<string> classes = pragmas.getClassesNeeded()->getNames();
	set<string>::const_iterator classIt = classes.begin();
	SgClassSymbol* classSym = NULL;
	for(classIt = classes.begin(); classIt != classes.end(); classIt++)
	{
		classSym = lookupClassSymbolInParentScopes(*classIt, getScope(kernel));
		ROSE_ASSERT(classSym != NULL);
		SgClassType* classType = isSgClassType(classSym->get_type());
		ROSE_ASSERT(classType != NULL);
		partition->addClassDeclaration(classType);
	}

	//Make an MPI stub
	SgTreeCopy deepCopy;
	SgName mpiStubName(NAME(stub) + "_mpi");
	SgFunctionParameterList* mpiParams =
			isSgFunctionParameterList(kernel->get_parameterList()->copy(deepCopy));
	mpiStub = buildDefiningFunctionDeclaration(mpiStubName, stub->get_orig_return_type(),
			mpiParams, stub->get_scope(), stub->get_decoratorList());
	insertStatement(pragmas.getFirstPragma(), mpiStub);

	//Add switch case to original stub
	vector<SgExpression*> params;
	SgInitializedNamePtrList& kernelParams = stub->get_parameterList()->get_args();
	SgInitializedNamePtrList::const_iterator paramIt = kernelParams.begin();
	for(paramIt = kernelParams.begin(); paramIt != kernelParams.end(); paramIt++)
		params.push_back(buildVarRefExp(*paramIt, stub->get_definition()));
	SgExprListExp* callParams = buildExprListExp(params);
	SgCaseOptionStmt* mpiCase = buildCallCase(mpiStub, callParams, buildUnsignedLongVal(MPI));
	switchStmt->append_case(mpiCase);

	//Declare status variable
	SgVariableDeclaration* statusVar = buildVariableDeclaration("__mpi_status__",
			buildOpaqueType("MPI_Status", mpiStub->get_scope()), NULL, mpiStub->get_definition());
	mpiStub->get_definition()->append_statement(statusVar);
	status = buildInitializedName("__mpi_status__",
			buildOpaqueType("MPI_Status", mpiStub->get_scope()));

	//Declare & send function number
	SgAssignInitializer* funcNumInit = buildAssignInitializer(buildUnsignedLongVal(funcCode),
			buildUnsignedLongType());
	SgVariableDeclaration* funcNumBufVar = buildVariableDeclaration("__func_num__",
			buildIntType(), funcNumInit, mpiStub->get_definition());
	mpiStub->get_definition()->append_statement(funcNumBufVar);
	SgInitializedName* funcNumBuf = buildInitializedName("__func_num__", buildIntType());
	SgExprStatement* sendFuncNum = MPICallBuilder::buildMPISend(funcNumBuf,
			mpiStub->get_definition(), MPI);
	mpiStub->get_definition()->append_statement(sendFuncNum);

	//Add data transfer calls
	addMPITransferCalls(partition, funcCode, pragmas.getInputs()->getNames(), INPUTS);
	addMPITransferCalls(partition, funcCode, pragmas.getGlobalInputs()->getNames(), GLOBAL_INPUTS);
	addMPITransferCalls(partition, funcCode, pragmas.getOutputs()->getNames(), OUTPUTS);
	addMPITransferCalls(partition, funcCode, pragmas.getGlobalOutputs()->getNames(), GLOBAL_OUTPUTS);

	//Finalize
	partition->finalizeFunction(funcCode);

	partitions.insert(MPI_STR);
	string msg = "Added MPI implementation for " + stub->get_name();
	DEBUG(TOOL, msg);
}

/*
 * Copy functions used by this kernel to the specified partition.
 */
void PartitionKernel::copySupportingFunctions(Partition* partition)
{
	string msg = "";
	set<string> funcs = pragmas.getFuncsNeeded()->getNames();
	set<string>::const_iterator funcIt = funcs.begin();
	SgFunctionSymbol* funcSym = NULL;
	SgFunctionDeclaration* func = NULL;
	for(funcIt = funcs.begin(); funcIt != funcs.end(); funcIt++)
	{
		funcSym = lookupFunctionSymbolInParentScopes(*funcIt, kernel->get_scope());
		ROSE_ASSERT(funcSym);

		//TODO for now, assume that all supporting functions are defined
		func = isSgFunctionDeclaration(funcSym->get_declaration()->get_definingDeclaration());
		ROSE_ASSERT(func);
		partition->moveSupportingFunction(func);
	}
}

/*
 * Add calls to transfer data to/from an MPI partition for all variables in the
 * specified list.
 */
void PartitionKernel::addMPITransferCalls(MPIPartition* partition, Partition::FCode funcCode,
		set<string> vars, enum TransferType ttype)
{
	set<string>::const_iterator varIt = vars.begin();
	vector<SgStatement*> stmts;
	SgScopeStatement* stubDef = mpiStub->get_definition();
	string msg;

	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		string name(*varIt);
		if(name.rfind('*') != string::npos)
			name.resize(name.length() - 1);

		addMPITransferForVar(partition, funcCode, stmts, stubDef, ttype, name);
	}

	insertStatementList(getLastStatement(stubDef), stmts, false);
}

/*
 * Add MPI transfer calls for a specific variable.
 */
void PartitionKernel::addMPITransferForVar(MPIPartition* partition, Partition::FCode funcCode,
		vector<SgStatement*>& stmts, SgScopeStatement* stubDef, enum TransferType ttype, string& name)
{
	SgVariableSymbol* varSymbol = NULL;
	SgInitializedName* var = NULL;
	SgType* type = NULL;
	SgType* baseType = NULL;
	int numDimensions = 0;
	SgExprStatement* stmt = NULL;
	string msg;

	if(name == "__retval__")
	{
		//TODO
		ERROR(TOOL, "Cannot handle return values at this time!");
		ROSE_ASSERT(false);
	}

	//Find variable from name
	varSymbol = lookupVariableSymbolInParentScopes(name, kernel->get_definition());
	ROSE_ASSERT(varSymbol);
	var = varSymbol->get_declaration();
	type = var->get_type();

	if(isConstType(type))
	{
		//Don't need to send, simply copy var & definition to partition
		if(ttype == INPUTS)
			partition->addInput(funcCode, var);
		else
			partition->addGlobalInput(funcCode, var);
		return;
	}

	//Get type (and base type, if necessary), removing typedefs
	Misc::getType(&type, &baseType, &numDimensions);

	//Add MPI communication
	SgExpression* sizeCall = NULL, *sizeOfExp = NULL, *divideExp = NULL, *sizeExp = NULL;
	SgInitializer* sizeInit = NULL;
	SgVariableSymbol* sizeVarSymbol = NULL;
	if(ttype == INPUTS || ttype == GLOBAL_INPUTS)
	{
		msg = "\tAdding send calls for " + name;
		DEBUG(TOOL, msg);

		if(baseType)
		{
			if(numDimensions > 1)
			{
				//TODO for now, we only accept double pointers
				ROSE_ASSERT(numDimensions == 2);

				//TODO for now, our base type cannot be classes
				ROSE_ASSERT(!isSgClassType(baseType));

				//Declare outer size
				SgName outerSizeName(name + OUTERSIZE_NAME);
				sizeCall = MMCallBuilder::buildGetSizeCallExp(buildVarRefExp(var), stubDef);
				sizeOfExp = buildSizeOfOp(buildPointerType(baseType));
				divideExp = buildDivideOp(sizeCall, sizeOfExp);
				sizeInit = buildAssignInitializer(divideExp, buildUnsignedLongType());
				SgVariableDeclaration* outerSizeDecl = buildVariableDeclaration(outerSizeName,
						buildUnsignedLongType(), sizeInit, stubDef);
				stmts.push_back(outerSizeDecl);

				//Declare inner size
				SgName innerSizeName(name + INNERSIZE_NAME);
				SgPntrArrRefExp* arrRef = buildPntrArrRefExp(buildVarRefExp(var, stubDef),
						buildUnsignedLongVal(0));
				sizeCall = MMCallBuilder::buildGetSizeCallExp(arrRef, stubDef);
				sizeOfExp = buildSizeOfOp(baseType);
				SgMultiplyOp* rowSize = buildMultiplyOp(sizeOfExp, buildVarRefExp(outerSizeDecl));
				divideExp = buildDivideOp(sizeCall, rowSize);
				sizeInit = buildAssignInitializer(divideExp, buildUnsignedLongType());
				SgVariableDeclaration* innerSizeDecl = buildVariableDeclaration(innerSizeName,
						buildUnsignedLongType(), sizeInit, stubDef);
				stmts.push_back(innerSizeDecl);

				//Declare loop iterator
				/*SgName loopItName(name + LOOPIT_NAME);
				SgVariableDeclaration* loopItDecl = buildVariableDeclaration(loopItName,
						buildUnsignedLongType(), NULL, stubDef);
				stmts.push_back(loopItDecl);*/

				SgInitializedName* outerSizeVar = buildInitializedName(outerSizeName,
						buildUnsignedLongType());
				SgInitializedName* innerSizeVar = buildInitializedName(innerSizeName,
						buildUnsignedLongType());
				//SgVarRefExp* loopVarRef = buildVarRefExp(loopItDecl);

				//Send outer size -> Note: don't send outer array, it is only pointers
				stmt = MPICallBuilder::buildMPISend(outerSizeVar, stubDef, MPI);
				stmts.push_back(stmt);

				//Send inner size
				stmt = MPICallBuilder::buildMPISend(innerSizeVar, stubDef, MPI);
				stmts.push_back(stmt);

				//Send array
				SgPntrArrRefExp* hiddenArr = buildPntrArrRefExp(buildVarRefExp(var, stubDef),
						buildUnsignedLongVal(0));
				SgMultiplyOp* totalSize = buildMultiplyOp(buildVarRefExp(outerSizeVar, stubDef),
						buildVarRefExp(innerSizeVar, stubDef));
				stmt = MPICallBuilder::buildMPISend(hiddenArr, stubDef, MPI, totalSize);
				stmts.push_back(stmt);

				/* Build for-loop to send rest of array */
				/*SgBasicBlock* body = buildBasicBlock();

				//Init, test & update statements
				SgExprStatement* init = buildAssignStatement(loopVarRef, buildUnsignedLongVal(0));
				SgExprStatement* test = buildExprStatement(buildLessThanOp(loopVarRef,
						buildVarRefExp(outerSizeDecl)));
				SgExpression* update = buildPlusPlusOp(loopVarRef); */

				/* Build for-loop body */

				//Send inner input
				/*arrRef = buildPntrArrRefExp(buildVarRefExp(var, stubDef), loopVarRef);
				body->append_statement(MPICallBuilder::buildMPISend(arrRef,
						stubDef, MPI, buildVarRefExp(innerSizeVar)));

				///////////////////////////////////////////////////////////////
				//Note: the following code gets the internal size every loop iteration
				//sizeCall = MMCallBuilder::buildGetSizeCallExp(arrRef, stubDef);
				//sizeOfExp = buildSizeOfOp(baseType);
				//divideExp = buildDivideOp(sizeCall, sizeOfExp);
				//body->append_statement(buildAssignStatement(buildVarRefExp(innerSizeDecl), divideExp));

				//Send size
				//body->append_statement(MPICallBuilder::buildMPISend(innerSizeVar, stubDef, MPI));
				///////////////////////////////////////////////////////////////

				//Add for-loop
				stmts.push_back(buildForStatement(init, test, update, body));*/
			}
			else
			{
				//Declare size buffer
				SgName sizeName(name + SIZE_NAME);
				sizeCall = MMCallBuilder::buildGetSizeCallExp(buildVarRefExp(var), stubDef);
				sizeOfExp = buildSizeOfOp(baseType);
				divideExp = buildDivideOp(sizeCall, sizeOfExp);
				sizeInit = buildAssignInitializer(divideExp, buildUnsignedLongType());
				SgVariableDeclaration* sizeDecl = buildVariableDeclaration(sizeName,
						buildUnsignedLongType(), sizeInit, stubDef);
				SgInitializedName* sizeVar = buildInitializedName(sizeName, buildUnsignedLongType());
				stmts.push_back(sizeDecl);

				//Send size
				stmt = MPICallBuilder::buildMPISend(sizeVar, stubDef, MPI);
				stmts.push_back(stmt);

				//Send input
				if(isSgClassType(baseType))
					addClassMPITransferCalls(partition, stmts, stubDef, INPUTS, var, type, baseType);
				else
				{
					stmt = MPICallBuilder::buildMPISend(var, stubDef, MPI, sizeVar);
					stmts.push_back(stmt);
				}
			}
		}
		else if(isSgClassType(type))
			addClassMPITransferCalls(partition, stmts, stubDef, INPUTS, var, type);
		else
		{
			//Send input
			stmt = MPICallBuilder::buildMPISend(var, mpiStub->get_definition(), MPI);
			stmts.push_back(stmt);
		}

		//Add input to partition
		if(ttype == INPUTS)
			partition->addInput(funcCode, var);
		else
			partition->addGlobalInput(funcCode, var);
	}
	else
	{
		msg = "\tAdding receive calls for " + name;
		DEBUG(TOOL, msg);

		if(baseType)
		{
			if(numDimensions > 1)
			{
				//TODO for now, we only accept double pointers
				ROSE_ASSERT(numDimensions == 2);

				//TODO for now, our base type cannot be classes
				ROSE_ASSERT(!isSgClassType(baseType));

				//Check for previously saved size
				string sizeName = name + OUTERSIZE_NAME;
				sizeVarSymbol = lookupVariableSymbolInParentScopes(sizeName, stubDef);

				SgVariableSymbol* innerSizeSymbol = NULL/*, *itSymbol = NULL*/;
				if(!sizeVarSymbol)
				{
					//TODO declare size buffers & get sizes
				}
				else
				{
					//Find inner size declaration
					string innerSizeName = name + INNERSIZE_NAME;
					innerSizeSymbol = lookupVariableSymbolInParentScopes(innerSizeName, stubDef);
					ROSE_ASSERT(innerSizeSymbol != NULL);

					//Find loop iterator
					/*string itName = name + LOOPIT_NAME;
					itSymbol = lookupVariableSymbolInParentScopes(itName, stubDef);
					ROSE_ASSERT(itSymbol != NULL);*/
				}

				//Receive data
				SgPntrArrRefExp* arrRef = buildPntrArrRefExp(buildVarRefExp(var, stubDef),
						buildUnsignedLongVal(0));
				SgMultiplyOp* totalSize = buildMultiplyOp(buildVarRefExp(sizeVarSymbol),
						buildVarRefExp(innerSizeSymbol));
				stmts.push_back(MPICallBuilder::buildMPIReceive(arrRef, stubDef, MPI,
						status, totalSize));

				/* Build for-loop */
				//Initialization, test & update statements
				/*SgExprStatement* init = buildAssignStatement(buildVarRefExp(itSymbol),
						buildUnsignedLongVal(0));
				SgExprStatement* test = buildExprStatement(buildLessThanOp(buildVarRefExp(itSymbol),
						buildVarRefExp(sizeVarSymbol)));
				SgExpression* update = buildPlusPlusOp(buildVarRefExp(itSymbol));

				//Build loop body
				SgBasicBlock* body = buildBasicBlock();
				SgExpression* arrRef = buildPntrArrRefExp(buildVarRefExp(var, stubDef),
						buildVarRefExp(itSymbol));
				body->append_statement(MPICallBuilder::buildMPIReceive(arrRef, stubDef,
						MPI, status, buildVarRefExp(innerSizeSymbol)));*/

				///////////////////////////////////////////////////////////////
				//Note: this gets the internal size every loop iteration
				//sizeCall = MMCallBuilder::buildGetSizeCallExp(arrRef, stubDef);
				//sizeOfExp = buildSizeOfOp(baseType);
				//divideExp = buildDivideOp(sizeCall, sizeOfExp);
				//body->append_statement(buildAssignStatement(
				//		buildVarRefExp(innerSizeSymbol),divideExp));
				///////////////////////////////////////////////////////////////

				//stmts.push_back(buildForStatement(init, test, update, body, NULL));
			}
			else
			{
				//Check for previously saved size
				string sizeName = name + SIZE_NAME;
				sizeVarSymbol = lookupVariableSymbolInParentScopes(sizeName, stubDef);

				if(!sizeVarSymbol)
				{
					//Size was not previously declared.  Get size from wrapper
					//TODO is this ever the case?
					SgExpression* sizeCall =
							MMCallBuilder::buildGetSizeCallExp(buildVarRefExp(varSymbol), stubDef);
					SgExpression* sizeOfExp = buildSizeOfOp(baseType);
					sizeExp = buildDivideOp(sizeCall, sizeOfExp);
				}
				else
					sizeExp = buildVarRefExp(sizeVarSymbol);

				if(isSgClassType(baseType))
					addClassMPITransferCalls(partition, stmts, stubDef, OUTPUTS, var, type, baseType);
				else
				{
					stmt = MPICallBuilder::buildMPIReceive(var, stubDef, MPI, status, sizeExp);
					stmts.push_back(stmt);
				}
			}
		}
		else if(isSgClassType(type))
			addClassMPITransferCalls(partition, stmts, stubDef, OUTPUTS, var, type);
		else
		{
			//Receive output
			stmt = MPICallBuilder::buildMPIReceive(var, stubDef, MPI, status);
			stmts.push_back(stmt);
		}

		//Add output to partition
		if(ttype == OUTPUTS)
			partition->addOutput(funcCode, var);
		else
			partition->addGlobalOutput(funcCode, var);
	}
}

/*
 * Add all the calls necessary to send a struct over MPI.
 */
void PartitionKernel::addClassMPITransferCalls(MPIPartition* partition, vector<SgStatement*>& stmts,
		SgScopeStatement* stubDef, enum TransferType ttype, SgInitializedName* var, SgType* type,
		SgType* baseType)
{
	string msg;
	SgClassType* curType = isSgClassType(baseType != NULL ? baseType : type);
	ROSE_ASSERT(curType);

	SgExpression* size = NULL;
	SgVarRefExp* datatypeVar = NULL;
	bool containsPointers = Misc::classContainsPointers(curType);
	string name = curType->get_name() + "__datatype";

	//If not previously defined, define the datatype here
	if(mpiDatatypes.find(name) == mpiDatatypes.end())
		defineMPIDatatype(partition, name, curType, containsPointers);

	if(ttype == INPUTS || ttype == GLOBAL_INPUTS)
	{
		if(!containsPointers)
		{
			map<string, SgVariableDeclaration*>::const_iterator varIt =
					mpiDatatypes.find(NAME(curType) + "__datatype");
			ROSE_ASSERT(varIt != mpiDatatypes.end());
			datatypeVar = buildVarRefExp(varIt->second);

			//Send struct
			SgVariableSymbol* sizeVar =
						lookupVariableSymbolInParentScopes(NAME(var) + SIZE_NAME, stubDef);
			if(sizeVar)
				size = buildVarRefExp(sizeVar);
			stmts.push_back(MPICallBuilder::buildMPISend(var, stubDef, MPI, datatypeVar, size));
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
					mpiDatatypes.find(curType->get_name() + "__datatype");
			ROSE_ASSERT(varIt != mpiDatatypes.end());
			datatypeVar = buildVarRefExp(varIt->second);

			//Receive struct
			SgVariableSymbol* sizeVar =
						lookupVariableSymbolInParentScopes(var->get_name() + "__size", stubDef);
			if(sizeVar)
				size = buildVarRefExp(sizeVar);
			stmts.push_back(MPICallBuilder::buildMPIReceive(var, stubDef, MPI, status,
					datatypeVar, size));
		}
		else
		{
			//TODO handle structs w/ pointers
		}
	}
}

/*
 * Declare a variable + statements necessary to define a new MPI datatype
 * describing a struct to send/receive.
 */
void PartitionKernel::defineMPIDatatype(MPIPartition* partition, SgName name, SgClassType* type,
		bool containsPointers)
{
	vector<SgStatement*> stmts;
	string msg;
	SgClassDeclaration* classDecl =
			isSgClassDeclaration(type->get_declaration()->get_definingDeclaration());

	//Check for previous definition
	if(mpiDatatypes.find(name) != mpiDatatypes.end())
		return;

	//Define a function to declare all types
	if(!datatypeDefFunc)
	{
		DEBUG(TOOL, "Creating a function to define all MPI datatypes");

		datatypeDefFunc = buildDefiningFunctionDeclaration("__defineMPIDatatypes", buildVoidType(),
				buildFunctionParameterList(), getGlobalScope(kernel));
		datatypeDefBody = datatypeDefFunc->get_definition();

		SgFunctionDeclaration* main = findMain(getProject());
		insertStatement(main, datatypeDefFunc);
		SgExprStatement* funcCall = buildFunctionCallStmt(datatypeDefFunc->get_name(),
				buildVoidType(), NULL, main->get_definition());
		insertStatement(getFirstStatement(main->get_definition()), funcCall);
	}

	//Add statements to declare datatype
	if(!containsPointers)
	{
		//Class doesn't contain pointers, can define datatype to send
		msg = "\tDefining MPI datatype for " + type->get_name();
		DEBUG(TOOL, msg);

		//Declare datatype var
		SgVariableDeclaration* datatype = buildVariableDeclaration(name,
				buildOpaqueType("MPI_Datatype", classDecl->get_scope()), NULL,
				classDecl->get_scope());

		//If the declaration is in a header, for now we just define it before
		//the datatype function
		if(classDecl->get_file_info()->isOutputInCodeGeneration())
			insertStatement(classDecl, datatype, false);
		else
			insertStatement(datatypeDefFunc, datatype);
		mpiDatatypes.insert(pair<string, SgVariableDeclaration*>(name, datatype));

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
 * Build a case option that only calls the specified function.
 */
SgCaseOptionStmt* PartitionKernel::buildCallCase(SgFunctionDeclaration* function, SgExprListExp* params,
		SgExpression* key)
{
	return buildCaseOptionStmt(key, buildCaseBody(function, params));
}

/*
 * Build a default case option that only calls the specified function.
 */
SgDefaultOptionStmt* PartitionKernel::buildDefaultCallCase(SgFunctionDeclaration* function,
		SgExprListExp* params)
{
	return buildDefaultOptionStmt(buildCaseBody(function, params));
}

/*
 * Build a basic block with a call to the specified function with the
 * specified parameters.
 */
SgBasicBlock* PartitionKernel::buildCaseBody(SgFunctionDeclaration* function, SgExprListExp* params)
{
	SgStatement* kernelCall;

	//Build function call by name rather than getting symbol (building by symbol sometimes fails)
	SgFunctionCallExp* kernelCallExp = buildFunctionCallExp(NAME(function),
			function->get_orig_return_type(), params, isSgScopeStatement(switchStmt->get_body()));
	if(!isSgTypeVoid(function->get_orig_return_type()))
	{
		kernelCall = buildReturnStmt(kernelCallExp);
		return buildBasicBlock(kernelCall);
	}
	else
	{
		kernelCall = buildExprStatement(kernelCallExp);
		return buildBasicBlock(kernelCall, buildBreakStmt());
	}
}

/*
 * Annotate a kernel with a pragma detailing the architectures onto which a
 * kernel was partitioned.
 */
void PartitionKernel::annotate()
{
	string msg = "Annotating function " + NAME(stub);
	DEBUG(TOOL, msg);
	SgPragmaDeclaration* partitioned = PragmaBuilder::buildPragma(PARTITIONED, partitions,
			stub->get_scope());
	insertStatement(stub, partitioned);
}
