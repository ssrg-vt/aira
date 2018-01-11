/*
 * function_declaration.cpp
 *
 *  Created on: Jan 31, 2013
 *      Author: rlyerly
 */

/* Must come first for PCH to work */
#include "rose.h"

/* Standard library headers */
#include <iostream>

/* Partitioner headers */
#include "function_declaration.h"
#include "function_traversal.h"
#include "mpi_call_builder.h"
#include "partitioning_constants.h"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

/*
 * Constructor for the FunctionDeclaration wrapper.  Handles all initialization and performs some checks
 * to make sure a function can be partitioned.
 */
FunctionDeclaration::FunctionDeclaration(SgFunctionDeclaration* functionDeclaration, bool doPartition) :
	functionDeclaration(functionDeclaration),
	functionDefinition(functionDeclaration->get_definition()),
	selectedForPartitioning(doPartition),
	addReturnBuffer(false)
{
	initialize();
}

/*
 * Perform all initialization and analysis for the function declaration
 */
void FunctionDeclaration::initialize()
{
	partitionStatus = READY_FOR_PARTITIONING;

	//Make sure function has been selected for partitioning
	if(!selectedForPartitioning)
	{
		partitionStatus = NOT_SELECTED_FOR_PARTITIONING;
		return;
	}

	//Function definition must be visible
	if(!functionDefinition)
	{
		partitionStatus = NO_DEFINITION;
		return;
	}
	functionBody = functionDefinition->get_body();

	//Discover the function interface
	if(!findFunctionInterface())
	{
		return;
	}
}

/*
 * Discovers the function's interface, including:
 * 	-all inputs (including global variables used)
 * 	-all outputs (including global variables written and function arguments w/ side-effects)
 * 	-functions called by this function
 * 	-user-defined datatypes used by this function TODO
 */
bool FunctionDeclaration::findFunctionInterface()
{
	inputs = &functionDeclaration->get_parameterList()->get_args();

	//Check the list of inputs for arrays and edit the parameter list to include that array's dimensions
	SgInitializedNamePtrList::iterator it;
	for(it = inputs->begin(); it != inputs->end(); it++)
	{
		SgType* paramType = (*it)->get_type();
		if(isSgArrayType(paramType))
		{
			SgInitializedName* size = buildInitializedName((*it)->get_name().getString() + "_size",
					buildUnsignedLongType());
			it = inputs->insert(it+1, size);
		}
		else if(isSgPointerType(paramType))
		{
			//TODO handle pointers.  For now, we'll just say the function isn't partitionable
			partitionStatus = INVALID_INTERFACE;
			return false;
		}
	}

	//Create internal list of outputs
	if(!isSgTypeVoid(functionDeclaration->get_orig_return_type()))
	{
		string retValName = functionDeclaration->get_name().getString() + "_" +
				functionDeclaration->get_orig_return_type()->unparseToString();
		SgInitializedName* retVal = buildInitializedName(retValName,
				functionDeclaration->get_orig_return_type());
		outputs.push_back(retVal);
		addReturnBuffer = true;
	}

	//Traverse the function definition and find global variables
	FunctionTraversal ft(this);
	ft.traverse(functionDeclaration);

	//Check to make sure we don't call undefined functions that are not on our white-list
	if(ft.callsUndefinedFunctions())
	{
		partitionStatus = CONTAINS_FUNC_CALLS_WO_DEFINITION;
		return false;
	}
	else
		return true;
}

/*
 * Move the function wrapped by this class to the specified partition.
 */
bool FunctionDeclaration::partition(Partition* partition)
{
	MPICallBuilder callBuilder;
	SgExprStatement* mpiCall = NULL;
	SgName statusName("status");
	SgVariableDeclaration* statusDecl = NULL;
	SgVariableDeclaration* receiveBufDecl = NULL;
	SgReturnStmt* returnStmt = NULL;
	SgInitializedNamePtrList::const_iterator it;
	Rose_STL_Container<SgNode*>::const_iterator returnStmtIt;

	//Make sure we can partition
	if(!canPartition())
		return false;

	//Create the partitioned function - this is where the processing will take place on the accelerator
	//TODO this all goes on the "create_partition" side
	/* -------------------------------- Start ------------------------------ */
	//SgName newFuncName = "partitioned_" + this->functionDeclaration->get_name().getString();
	/* TODO
	 * Alastair -> you'll have your own version of "partitionedFunctionXXX",
	 * change all these variable names
	 */
	/*this->partitionedFunctionDeclaration = buildDefiningFunctionDeclaration(newFuncName,
			buildVoidType(), buildFunctionParameterList(),*/
			/* TODO
			 * Alastair -> set the scope to your partition's global scope
			 */
			/*this->functionDeclaration->get_scope());
	this->partitionedFunctionDefinition = this->partitionedFunctionDeclaration->get_definition();
	this->partitionedFunctionBody = this->partitionedFunctionDefinition->get_body();
	for(SgStatementPtrList::const_iterator bodyIt =	this->functionBody->get_statements().begin();
			bodyIt != this->functionBody->get_statements().end();
			bodyIt++)
	{
		this->partitionedFunctionBody->append_statement(*bodyIt);
	}*/
	/* TODO
	 * Alastair -> insert the function into your partition
	 */
	//appendStatement(partitionedFunctionDeclaration, this->functionDeclaration->get_scope());


	//The following code handles data transfer to/from the partitioned function
	//on the device.  Insert MPI calls to receive data from the host.
	/* TODO
	 * Alastair -> don't worry about iterating through the lists (that's the
	 * hosts' job).  All you need to copy is the MPICallBuilder stuff
	 */
	/*if((inputs->begin() != inputs->end()) || (globalInputs.begin() != globalInputs.end())) {
		//Declare a status variable used when calling MPI receive functions
		statusDecl = buildVariableDeclaration(statusName, buildOpaqueType("MPI_Status",
				getGlobalScope(partitionedFunctionBody)), NULL, partitionedFunctionBody);
	}
	for(it = inputs->begin(); it != inputs->end(); it++) {
		receiveBufDecl = buildVariableDeclaration((*it)->get_name(), (*it)->get_type(),
				NULL, partitionedFunctionBody);
		partitionedFunctionBody->prepend_statement(receiveBufDecl);
		mpiCall = callBuilder.buildMPIReceive(*it, partition->getPartitionNumber(),
				partitionedFunctionBody, statusDecl->get_decl_item(statusName));
		insertStatement(receiveBufDecl, mpiCall, false);
	}
	for(it = globalInputs.begin(); it != globalInputs.end(); it++) {
		//TODO handle passing arrays
		receiveBufDecl = buildVariableDeclaration((*it)->get_name(), (*it)->get_type(),
				NULL, partitionedFunctionBody);
		partitionedFunctionBody->prepend_statement(receiveBufDecl);
		mpiCall = callBuilder.buildMPIReceive(*it, partition->getPartitionNumber(),
				partitionedFunctionBody, statusDecl->get_decl_item(statusName));
		insertStatement(receiveBufDecl, mpiCall, false);
	}
	if(statusDecl)
		partitionedFunctionBody->prepend_statement(statusDecl);

	//Insert MPI calls to send data back to the host
	Rose_STL_Container<SgNode*> returnStmtList =
			NodeQuery::querySubTree(partitionedFunctionDeclaration, V_SgReturnStmt);
	if(returnStmtList.begin() != returnStmtList.end()) {
		//The function has return statements
		for(returnStmtIt = returnStmtList.begin();
				returnStmtIt != returnStmtList.end();
				returnStmtIt++)
		{
			SgReturnStmt* returnStmt = isSgReturnStmt(*returnStmtIt);
			ROSE_ASSERT(returnStmt != NULL);

			//Global values that were written always need to be transferred
			//back to the host
			for(it = globalOutputs.begin(); it != globalOutputs.end(); it++) {
				//TODO handle sending back arrays
				mpiCall = callBuilder.buildMPISend(*it, getScope(partitionedFunctionBody),
						HOST);
				insertStatement(returnStmt, mpiCall);
			}

			//See if the expression in the return statement needs to be lifted
			//into a temporary variable
			//TODO handle passing back arrays
			SgExpression* returnExp = returnStmt->get_expression();
			if(returnExp) {
				SgVarRefExp* returnVar = isSgVarRefExp(returnExp);
				if(returnVar) {
					//If the return expression is a variable reference, we can simply
					//perform an MPI call to send that data back
					mpiCall = callBuilder.buildMPISend(returnVar->get_symbol()->get_declaration(),
							getScope(partitionedFunctionBody), HOST);
					insertStatement(returnStmt, mpiCall);
				} else {
					//Not a variable reference.  Lift the return expression out of
					//the return statement and store it in a temporary variable to be
					//sent back via MPI call
					SgAssignInitializer* returnBufInitializer = buildAssignInitializer(
							returnExp, output->get_type());
					SgVariableDeclaration* returnBufDecl = buildVariableDeclaration(
							output->get_name(), output->get_type(),
							returnBufInitializer, partitionedFunctionBody);
					insertStatement(returnStmt, returnBufDecl);
					mpiCall = callBuilder.buildMPISend(
							returnBufDecl->get_decl_item(output->get_name()),
							getScope(partitionedFunctionBody), HOST);
					insertStatement(returnStmt, mpiCall);
				}
			}
			//Remove the return statement, as it has been replaced by MPI calls
			//TODO AST rooted at return statement still in memory...purge from memory?
			removeStatement(returnStmt);
		}
	} else {
		//There are no return statements in the function, just add MPI calls
		//for the global variables written to the end of the function
		//TODO tentatively assume user has done this correctly (i.e. it is a void return type)
		for(it = globalOutputs.begin(); it != globalOutputs.end(); it++) {
			mpiCall = callBuilder.buildMPISend(*it, getScope(this->partitionedFunctionBody),
					HOST);
			this->partitionedFunctionBody->append_statement(mpiCall);
		}
	}*/
	/* --------------------------------- End ------------------------------- */

	//Move the function to the partition
	Partition::FCode functionNum = partition->moveFunction(functionDeclaration);

	//Change the current function into a stub that encapsulates data transfers
	SgBasicBlock* newFunctionBody = buildBasicBlock();
	functionDefinition->set_body(newFunctionBody);
	newFunctionBody->set_parent(functionDefinition);

	//Insert a call to tell the partition which function we want to execute
	SgAssignInitializer* functionNumInitializer = buildAssignInitializer(buildIntVal(functionNum),
			buildIntType());
	SgVariableDeclaration* functionNumVar = buildVariableDeclaration("__function_number", buildIntType(),
			functionNumInitializer, newFunctionBody);
	newFunctionBody->append_statement(functionNumVar);
	mpiCall = callBuilder.buildMPISend(buildInitializedName("__function_number", buildIntType(),
			functionNumInitializer), newFunctionBody, partition->getPartitionNumber());
	newFunctionBody->append_statement(mpiCall);

	//Insert MPI calls to transfer data to a partition
	for(it = inputs->begin(); it != inputs->end(); it++)
		addInput(partition, &it, newFunctionBody, functionNum);

	for(it = globalInputs.begin(); it != globalInputs.end(); it++)
		addInput(partition, &it, newFunctionBody, functionNum);

	//If we transfer anything back, declare a status variable
	if((outputs.size() > 0 ) || (globalOutputs.size() > 0))
	{
		statusDecl = buildVariableDeclaration(statusName, buildOpaqueType("MPI_Status", newFunctionBody),
				NULL, newFunctionBody);
		functionDefinition->append_statement(statusDecl);
	}

	//If returning a value from the function, declare a buffer to receive from the partition
	if(addReturnBuffer)
	{
		//TODO handle allocating memory if necessary (pointers)
		receiveBufDecl = buildVariableDeclaration((*outputs.begin())->get_name(),
				(*outputs.begin())->get_type(), NULL, newFunctionBody);
		functionDefinition->prepend_statement(receiveBufDecl);
		returnStmt = buildReturnStmt(buildVarRefExp(receiveBufDecl));
	}
	else
		returnStmt = buildReturnStmt();

	//Insert MPI calls to transfer data back from a partition
	for(it = outputs.begin(); it != outputs.end(); it++)
		addOutput(partition, &it, newFunctionBody, functionNum, statusDecl);

	for(it = globalOutputs.begin(); it != globalOutputs.end(); it++)
		addOutput(partition, &it, newFunctionBody, functionNum, statusDecl);

	functionDefinition->append_statement(returnStmt);

	partitionStatus = PARTITIONED;

	return true;
}

/*
 * Add an input to transfer data to a partition.
 */
void FunctionDeclaration::addInput(Partition* partition, SgInitializedNamePtrList::const_iterator* it,
		SgBasicBlock* newFunctionBody, int functionNum)
{
	MPICallBuilder callBuilder;
	SgExprStatement* mpiCall = NULL;

	if(isSgArrayType((**it)->get_type()) || isSgPointerType((**it)->get_type()))
	{
		//Send the array size over first
		mpiCall = callBuilder.buildMPISend(*(*it + 1), newFunctionBody, partition->getPartitionNumber());
		functionDefinition->append_statement(mpiCall);

		mpiCall = callBuilder.buildMPISend(**it, newFunctionBody, partition->getPartitionNumber(),
				*(*it + 1));
		functionDefinition->append_statement(mpiCall);

		//TODO after Alastair has changed his interface, pass an SgInitializedName*
		partition->addInput(functionNum, (*(*it + 1))->get_type());
		partition->addInput(functionNum, (**it)->get_type());
		(*it)++;
	}
	else
	{
		mpiCall = callBuilder.buildMPISend(**it, newFunctionBody, partition->getPartitionNumber());
		functionDefinition->append_statement(mpiCall);
		partition->addInput(functionNum, (**it)->get_type());
	}
}

/*
 * Add an input to transfer data back from a partition.
 */
void FunctionDeclaration::addOutput(Partition* partition, SgInitializedNamePtrList::const_iterator* it,
		SgBasicBlock* newFunctionBody, int functionNum, SgVariableDeclaration* statusDecl)
{
	MPICallBuilder callBuilder;
	SgExprStatement* mpiCall = NULL;
	SgName statusName("status");

	if(isSgArrayType((**it)->get_type()) || isSgPointerType((**it)->get_type()))
	{
		//Receive the array size first
		mpiCall = callBuilder.buildMPIReceive(*(*it + 1), newFunctionBody, partition->getPartitionNumber(),
				statusDecl->get_decl_item(statusName));
		functionDefinition->append_statement(mpiCall);

		mpiCall = callBuilder.buildMPIReceive(**it, newFunctionBody, partition->getPartitionNumber(),
				statusDecl->get_decl_item(statusName), *(*it + 1));
		functionDefinition->append_statement(mpiCall);

		//TODO after Alastair has changed his interface, pass an SgInitializedName*
		partition->addOutput(functionNum, (*(*it + 1))->get_type());
		partition->addOutput(functionNum, (**it)->get_type());
		(*it)++;
	}
	else
	{
		mpiCall = callBuilder.buildMPIReceive(**it, newFunctionBody, partition->getPartitionNumber(),
				statusDecl->get_decl_item(statusName));
		functionDefinition->append_statement(mpiCall);
		partition->addOutput(functionNum, (**it)->get_type());
	}
}

/*
 * Return a list of this function's inputs
 */
const SgInitializedNamePtrList& FunctionDeclaration::getInputs()
{
	return *inputs;
}

/*
 * Return a list of this function's outputs
 */
const SgInitializedNamePtrList& FunctionDeclaration::getOutputs()
{
	return outputs;
}

/*
 * Return a list of the global variables used in this function
 */
const SgInitializedNamePtrList& FunctionDeclaration::getGlobalInputs()
{
	return globalInputs;
}

/*
 * Return a list of the global variables written in this function
 */
const SgInitializedNamePtrList& FunctionDeclaration::getGlobalOutputs()
{
	return globalOutputs;
}

/*
 * Return whether or not this function can be partitioned (i.e. it is ready
 * for partitioning)
 */
bool FunctionDeclaration::canPartition()
{
	return (partitionStatus == READY_FOR_PARTITIONING);
}

/*
 * Containment checking for a list.  This is used to eliminate redundant inputs
 * or outputs
 *
 * TODO switch to hash maps for cleaner checking?
 */
bool FunctionDeclaration::contains(SgInitializedNamePtrList& list,
		SgInitializedName* var) {

	for(SgInitializedNamePtrList::const_iterator it = list.begin();
			it != list.end();
			it++)
	{
		if(*it == var)
			return true;
	}
	return false;
}

/*
 * Containment checking for a list.  This is used to eliminate redundant inputs
 * or outputs.
 *
 * TODO switch to hash maps for cleaner checking?
 */
bool FunctionDeclaration::contains(
		Rose_STL_Container<SgFunctionDeclaration*> list,
		SgFunctionDeclaration* var) {

	for(Rose_STL_Container<SgFunctionDeclaration*>::const_iterator it = list.begin();
			it != list.end();
			it++)
	{
		if(*it == var)
			return true;
	}
	return false;
}

/*
 * Helper function which prints this function's interface
 *
 * TODO print out functions called by this function & user-defined datatypes
 * utilized by this function
 */
void FunctionDeclaration::printFunctionInterface()
{
	SgInitializedNamePtrList::const_iterator it;
	SgName name;

	cout << "Function declaration: " << functionDeclaration->get_name() << endl;

	if(partitionStatus == NO_DEFINITION)
	{
		cout << "No definition" << endl << endl;
		return;
	}

	//Print the function inputs
	cout << "Inputs:" << endl;
	for(it = inputs->begin(); it != inputs->end(); it++)
	{
		//Print the type
		cout << "\t" << (*it)->get_type()->unparseToString();

		//Print the name, if it exists
		name = (*it)->get_name();
		if(name.getString().compare("") != 0)
			cout << " " << name.getString();
		cout << endl;
	}

	//Print the function outputs
	cout << "Outputs:" << endl;
	for(it = outputs.begin(); it != outputs.end(); it++)
	{
		cout << "\t" << (*it)->get_type()->unparseToString();

		name = (*it)->get_name();
		if(name.getString().compare("") != 0)
			cout << " " << name.getString();
		cout << endl;
	}

	//Print the global function inputs
	cout << "Global variables read:" << endl;
	for(it = globalInputs.begin(); it != globalInputs.end(); it++)
	{
		cout << "\t" << (*it)->get_type()->unparseToString();

		name = (*it)->get_name();
		if(name.getString().compare("") != 0)
			cout << " " << name.getString();
		cout << endl;
	}

	//Print the global function outputs
	cout << "Global variables written:" << endl;
	for(it = globalOutputs.begin(); it != globalOutputs.end(); it++)
	{
		cout << "\t" << (*it)->get_type()->unparseToString();

		name = (*it)->get_name();
		if(name.getString().compare("") != 0)
			cout << " " << name.getString();
		cout << endl;
	}

	cout << endl;
}

/*
 * Prints the partitioning status for the function.
 */
void FunctionDeclaration::printPartitioningStatus()
{
	string functionName = functionDeclaration->get_name().getString();
	switch(partitionStatus)
	{
	case NOT_SELECTED_FOR_PARTITIONING:
		cout << "Function " << functionName << " was not selected for partitioning" << endl;
		break;
	case READY_FOR_PARTITIONING:
		cout << "Function " << functionName << " is ready for partitioning" << endl;
		break;
	case PARTITIONED:
		cout << "Function " << functionName << " has been partitioned" << endl;
		break;
	case NO_DEFINITION:
		cout << "Function " << functionName << " cannot be partitioned - there is no definition, "
				"or it is not visible" << endl;
		break;
	case CONTAINS_FUNC_CALLS_WO_DEFINITION:
		cout << "Function " << functionName << " cannot be partitioned - it contains calls to functions"
				" without a definition" << endl;
		break;
	case INVALID_INTERFACE:
		cout << "Function " << functionName << " cannot be partitioned - it has an invalid interface"
				<< endl;
		break;
	default:
		cout << "Unknown partitioning status for function " << functionName << endl;
		break;
	}
	return;
}
