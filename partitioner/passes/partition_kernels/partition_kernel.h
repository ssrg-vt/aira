/*
 * partition_kernel.h
 *
 *  Created on: Apr 24, 2013
 *      Author: rlyerly
 */

#ifndef PARTITION_KERNEL_H_
#define PARTITION_KERNEL_H_

#include "create_partition.h"

class PartitionKernel {
public:
	PartitionKernel(SgFunctionDeclaration*, Pragmas&);
	void partitionToGPU(GPUPartition* partition);
	void partitionToMPI(MPIPartition* partition);
	void annotate();

private:
	SgFunctionDeclaration* kernel;
	SgFunctionDeclaration* stub;
	SgFunctionDeclaration* gpuPrototype;
	SgFunctionDeclaration* mpiStub;
	SgSwitchStatement* switchStmt;
	SgVariableDeclaration* partitionVar;
	SgInitializedName* status;
	set<string> partitions;
	Pragmas& pragmas;

	static map<string, SgVariableDeclaration*> mpiDatatypes;
	static SgFunctionDeclaration* datatypeDefFunc;
	static SgFunctionDefinition* datatypeDefBody;

	enum TransferType {
		INPUTS,
		GLOBAL_INPUTS,
		OUTPUTS,
		GLOBAL_OUTPUTS
	};

	void copySupportingFunctions(Partition* partition);
	void addMPITransferCalls(MPIPartition* partittion, Partition::FCode funcCode,
			set<string> vars, enum TransferType ttype);
	void addMPITransferForVar(MPIPartition* partition, Partition::FCode funcCode,
			vector<SgStatement*>& stmts, SgScopeStatement* stubDef, enum TransferType ttype,
			string& name);
	void addClassMPITransferCalls(MPIPartition* partition, vector<SgStatement*>& stmts,
			SgScopeStatement* stubDef, enum TransferType ttype, SgInitializedName* var,
			SgType* type, SgType* baseType = NULL);
	void defineMPIDatatype(MPIPartition* partition, SgName datatypeName, SgClassType* type,
			bool containsPointers);
	SgCaseOptionStmt* buildCallCase(SgFunctionDeclaration* function, SgExprListExp* params,
			SgExpression* key);
	SgDefaultOptionStmt* buildDefaultCallCase(SgFunctionDeclaration* function, SgExprListExp* params);
	SgBasicBlock* buildCaseBody(SgFunctionDeclaration* function, SgExprListExp* params);
};

#endif /* PARTITION_KERNEL_H_ */
