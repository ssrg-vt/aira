/*
 * function_declaration.h
 *
 *  Created on: Jan 31, 2013
 *      Author: rlyerly
 */

#ifndef FUNCTION_DECLARATION_H_
#define FUNCTION_DECLARATION_H_

#include "partition_status.h"
#include "create_partition.h"

class FunctionDeclaration
{
public:
	/*
	 * Default constructor which wraps the specified function declaration.
	 */
	FunctionDeclaration(SgFunctionDeclaration* functionDeclaration, bool doPartition);

	/*
	 * Getters to read the inputs/outputs to this function.  Global variables are handled slightly
	 * differently, so they are stored in separate lists.
	 */
	const SgInitializedNamePtrList& getInputs();
	const SgInitializedNamePtrList& getOutputs();
	const SgInitializedNamePtrList& getGlobalInputs();
	const SgInitializedNamePtrList& getGlobalOutputs();

	/*
	 * Partition the code.  Place the code on the partition(s) specified by the argument.
	 *
	 * TODO add to multiple partitions?
	 */
	bool partition(Partition* partition);

	/*
	 * Returns whether or not this function can be partitioned.
	 */
	bool canPartition();

	/*
	 * Programmer helper function that prints the function's interface.
	 */
	void printFunctionInterface();

	/*
	 * Programmer helper function that prints the partitioning status of the function.
	 */
	void printPartitioningStatus();

	/*
	 * Make the function traversal class a friend class so that it can use this class' variables.
	 */
	friend class FunctionTraversal;

private:
	/*
	 * Function declaration, definition and body.  These refer to the respective parts of the function
	 * being wrapped.
	 */
	SgFunctionDeclaration* functionDeclaration;
	SgFunctionDefinition* functionDefinition;
	SgBasicBlock* functionBody;

	/*
	 * Internal lists of inputs/outputs.
	 */
	SgInitializedNamePtrList* inputs; //Pain in the ass because this is a pointer - this is so we can use
									  //the argument list created by Rose
	SgInitializedNamePtrList outputs;
	SgInitializedNamePtrList globalInputs;
	SgInitializedNamePtrList globalOutputs;

	/*
	 * Internal lists of functions called from this function and user-defined data structures used in this
	 * function.  These must be transferred to the partition as well.
	 */
	Rose_STL_Container<SgFunctionDeclaration*> functionsCalled;
	//TODO user-defined data structures

	/*
	 * Various object members.
	 */
	bool selectedForPartitioning;
	bool addReturnBuffer;
	enum status partitionStatus;

	/*
	 * Checks to make sure the current function can be partitioned as well as prepare any internal data
	 * structures for partitioning.
	 */
	void initialize();

	/*
	 * Builds the function interface, which describes the data that needs to be transferred to and from
	 * the function.
	 */
	bool findFunctionInterface();

	/*
	 * Add a data transfer with respect to the specified partition
	 */
	void addInput(Partition* partition, SgInitializedNamePtrList::const_iterator* it,
			SgBasicBlock* newFunctionBody, int functionNum);
	void addOutput(Partition* partition, SgInitializedNamePtrList::const_iterator* it,
			SgBasicBlock* newFunctionBody, int functionNum, SgVariableDeclaration* statusDecl);

	/*
	 * Checks the passed list to see if it contains the passed variable.
	 */
	bool contains(SgInitializedNamePtrList& list, SgInitializedName* var);
	bool contains(Rose_STL_Container<SgFunctionDeclaration*> list, SgFunctionDeclaration* var);
};


#endif /* FUNCTION_DECLARATION_H_ */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
