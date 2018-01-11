/*
 * main.cpp
 *
 *  Created on: June 7, 2013
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */
#include "rose.h"

#include "common.h"
#include "add_scheduler_calls_common.h"
#include "program_options.h"
#include "pragma_handling.h"
#include "call_scheduler.h"

int main(int argc, char** argv)
{
	//Parse command-line args
	ProgramOptions po(argc, argv);

	//Initialize the AST
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project);
	AstTests::runAllTests(project); //TODO switch on/off with command-line args

	//Set the folder containing the features
	string featureFolder = "";
	bool defaultFeatures = po.getFeaturesFolder(featureFolder);
	if(!defaultFeatures)
		CallScheduler::setFeaturesFolder(featureFolder);

	//Loop through all partitioned kernels and add scheduling calls
	Rose_STL_Container<SgNode*> pragmas = querySubTree(project, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt = pragmas.begin();
	SgPragmaDeclaration* pragma = NULL;
	SgFunctionDeclaration* funcDecl = NULL;
	SgStatement* stmt = NULL;
	for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
	{
		pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma);

		PragmaParser pp(pragma);
		if(pp.isPopcornPragma() && pp.getPragmaType() == PARTITIONED)
		{
			stmt = getNextStatement(pragma);
			while(!isSgFunctionDeclaration(stmt))
				stmt = getNextStatement(pragma);
			funcDecl = isSgFunctionDeclaration(stmt);
			ROSE_ASSERT(funcDecl);
			Pragmas pragmas(funcDecl);

			//Add scheduling calls
			CallScheduler cs(funcDecl, pragmas);
			cs.addSchedulerCalls();

			//Insert the header
			//TODO this won't insert the header into files
			insertHeader(po.getSchedulerHeaderLocation(), PreprocessingInfo::after, false,
					getGlobalScope(funcDecl));
		}
	}

	return backend(project);
}
