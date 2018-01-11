/*
 * call_scheduler.h
 *
 *  Created on: Jun 7, 2013
 *      Author: rlyerly
 */

#ifndef CALL_SCHEDULER_H_
#define CALL_SCHEDULER_H_

class CallScheduler {
public:
	CallScheduler(SgFunctionDeclaration* p_function, Pragmas& p_pragmas);
	void addSchedulerCalls();

	static void setFeaturesFolder(string p_featuresFolder);

private:
	SgFunctionDeclaration* function;
	Pragmas& pragmas;
	SgSwitchStatement* switchStmt;
	SgVariableDeclaration* partNum;
	SgVariableDeclaration* kernelFeatures;
	string appFeaturesFolder;
	list<string> featureFiles;

	static string featuresFolder;
	static bool defaultFeaturesFolder;

	void addStructInitialization();
	void addDynamicFeatures();
	void addCallToScheduler();
	void addCallToCleanup();
	bool findFeatureFiles();

	int sumVarSizes(set<string> vars, SgFunctionDefinition* funcDef, bool isInput);
};

#endif /* CALL_SCHEDULER_H_ */
