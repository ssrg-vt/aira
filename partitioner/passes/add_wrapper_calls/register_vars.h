/*
 * register_vars.h
 *
 *  Created on: May 17, 2013
 *      Author: rlyerly
 */

#ifndef REGISTER_VARS_H_
#define REGISTER_VARS_H_

#define MMWRAPPER_HEADER "mm_wrapper.h"

class RegisterVars {
public:
	RegisterVars(SgProject* p_project, string p_mmWrapperHeader = MMWRAPPER_HEADER);
	void registerStaticVars();

	static void registerGlobalVars(set<SgInitializedName*>& globalVars);

private:
	SgProject* project;
	set<SgInitializedName*> registeredVars;
	string mmWrapperHeader;

	void registerAddressOfs();
	void registerArrays();
	void registerStructs();
};

#endif /* REGISTER_VARS_H_ */
