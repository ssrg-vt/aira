/*
 * sched_call_builder.h
 *
 *  Created on: June 7, 2013
 *	Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef SCHED_CALL_BUILDER_H_
#define SCHED_CALL_BUILDER_H_

enum Field {
	INSTR_STATIC,
	INSTR_DYNAMIC,
	SI_STATIC,
	SI_DYNAMIC,
	VI_STATIC,
	VI_DYNAMIC,
	SF_STATIC,
	SF_DYNAMIC,
	VF_STATIC,
	VF_DYNAMIC,
	SB_STATIC,
	SB_DYNAMIC,
	VB_STATIC,
	VB_DYNAMIC,
	LOAD_STATIC,
	LOAD_DYNAMIC,
	STORE_STATIC,
	STORE_DYNAMIC,
	CALLS_STATIC,
	CALLS_DYNAMIC,
	MATH_STATIC,
	MATH_DYNAMIC,
	CYCLOMATIC,
	BRANCH_STATIC,
	BRANCH_DYNAMIC,
	JUMP_STATIC,
	JUMP_DYNAMIC,
	PARALLEL_STATIC,
	PARALLEL_DYNAMIC,
	MEMORY_TX,
	MEMORY_RX,
	WORK_ITEMS
};

namespace SchedCallBuilder {
	//SgVariableDeclaration* buildArchInfo(SgScopeStatement* scope);
	SgVariableDeclaration* buildKernelFeatures(SgScopeStatement* scope);
	bool initializeFeaturesFromFiles(SgVariableDeclaration* features,
		list<string>& filenames, SgScopeStatement* scope);
	SgExprStatement* setField(SgVariableDeclaration* kernelFeatures,
		SgExpression* rhs, enum Field field);
	SgExprStatement* addToField(SgVariableDeclaration* kernelFeatures,
		SgExpression* rhs, enum Field field);
	SgExprStatement* buildSchedulerCall(SgVariableDeclaration*
		partitionNum, SgVariableDeclaration* kernelFeatures);
	SgExprStatement* buildCleanupCall(SgVariableDeclaration* kernelFeatures);
}

#endif /* SCHED_CALL_BUILDER_H_ */
