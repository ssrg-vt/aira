/*
 * register_vars.cpp
 *
 *  Created on: May 17, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "add_wrapper_calls_common.h"
#include "register_vars.h"
#include "register_pointers.h"

/*
 * Default constructor.
 */
RegisterVars::RegisterVars(SgProject* p_project, string p_mmWrapperHeader) :
	project(p_project),
	mmWrapperHeader(p_mmWrapperHeader)
{
	if(mmWrapperHeader == "")
		mmWrapperHeader = MMWRAPPER_HEADER;
}

/*
 * Add calls to register sizes of static variables in the MM-wrapper
 */
void RegisterVars::registerStaticVars()
{
	registerAddressOfs();
	registerArrays();
	registerStructs();
}

/*
 * Search for and add register/unregister calls for statically allocated
 * arrays.
 */
void RegisterVars::registerArrays()
{
	string msg = "Finding statically-allocated arrays";
	DEBUG(TOOL, msg);

	//Search all variables for declared arrays
	//TODO this is naive, maybe adds too much runtime overhead?
	Rose_STL_Container<SgNode*> vars = querySubTree(project, V_SgVariableDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator varIt;
	SgInitializedNamePtrList::const_iterator varNameIt;
	SgInitializedName* varName;
	SgVariableDeclaration* varDecl;
	SgType* type;
	set<SgInitializedName*>::const_iterator varFind;
	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		varDecl = isSgVariableDeclaration(*varIt);
		ROSE_ASSERT(varDecl);

		//Weed out struct/class members
		if(isSgClassDefinition(getScope(varDecl)))
			continue;

		SgInitializedNamePtrList& varNames = varDecl->get_variables();
		for(varNameIt = varNames.begin(); varNameIt != varNames.end(); varNameIt++)
		{
			varName = *varNameIt;
			type = varName->get_type();
			ROSE_ASSERT(varName->get_declaration());
			if(!isSgArrayType(type))
				continue;

			//Check the following criteria ((1 or 2) and 3):
			//	1. Haven't already added the variable OR
			//	2. Haven't already added a variable of the same name in the same scope
			//			AND
			//	3. Isn't a global variable
			varFind = registeredVars.find(varName);
			if((varFind == registeredVars.end() || getScope(*varFind) != getScope(varName))
					&& !isSgGlobal(getScope(varName->get_declaration())))
			{
				msg = "\tAdding calls to register/unregister \"" + NAME(varName) + "\"";
				DEBUG(TOOL, msg);

				RegisterPointers rp(varName);
				if(rp.addRegUnregCalls())
				{
					registeredVars.insert(varName);
					insertHeader(mmWrapperHeader, PreprocessingInfo::after, false,
							getGlobalScope(varDecl));
				}
			}
		}
	}
}

/*
 * Search for and add calls to register/unregister variables who have their
 * address taken.
 */
void RegisterVars::registerAddressOfs()
{
	string msg = "Finding variables who have their address taken";
	DEBUG(TOOL, msg);

	//Find address-of ops
	Rose_STL_Container<SgNode*> vars = querySubTree(project, V_SgAddressOfOp);
	Rose_STL_Container<SgNode*>::const_iterator varIt;
	SgAddressOfOp* addrOp;
	SgVarRefExp* varRef;
	SgInitializedName* varName;
	set<SgInitializedName*>::const_iterator varFind;
	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		addrOp = isSgAddressOfOp(*varIt);
		ROSE_ASSERT(addrOp);

		//TODO we only care about address-of ops of scalar variables at this point, right?
		varRef = isSgVarRefExp(addrOp->get_operand());
		if(varRef)
		{
			varName = varRef->get_symbol()->get_declaration();
			varFind = registeredVars.find(varName);
			if((varFind == registeredVars.end() || getScope(*varFind) != getScope(varName))
					&& !isSgGlobal(getScope(varName)))
			{
				msg = "\tAdding calls to register/unregister \"" + NAME(varName) + "\"";
				DEBUG(TOOL, msg);

				RegisterPointers rp(addrOp);
				if(rp.addRegUnregCalls())
				{
					registeredVars.insert(varName);
					insertHeader(mmWrapperHeader, PreprocessingInfo::after, false,
							getGlobalScope(addrOp));
				}
			}
		}
	}
}

/*
 * Register static struct declarations
 */
void RegisterVars::registerStructs()
{
	//TODO
}

/*
 * Register/unregister global variables in main.
 */
void RegisterVars::registerGlobalVars(set<SgInitializedName*>& globalVars)
{
	set<SgInitializedName*>::const_iterator globVarIt;
	string msg;
	for(globVarIt = globalVars.begin(); globVarIt != globalVars.end(); globVarIt++)
	{
		msg = "\tAdding calls to register/unregister \"" + NAME(*globVarIt) + "\"";
		DEBUG(TOOL, msg);

		RegisterPointers rp(*globVarIt, true);
		rp.addRegUnregCalls();
	}
}
