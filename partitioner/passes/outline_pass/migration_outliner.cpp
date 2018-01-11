/* Pre-Analysis phase function outlining program
 *
 * To break-up and have more things for analysis
 * Specifically: for-loops/OMP regions
 *
 * Created: 10/22/2014
 * Author: bielsk1
 */

#include "rose.h"
#include "common.h"
#include "migration_outliner.h"

#include <iostream>
#include <string.h>
#include <sstream>
#define TOOL ""


/* 
 * Default Constructor 
 */
MigrationOutliner::MigrationOutliner(SgProject* p){
	project = p;
	main_body = findMain(p);
	counter = 0;
}

/**************************** smaller utility Functions **********************************/
string convertInt(int number){
  stringstream ss;//create a stringstream
  ss << number;//add number to the stream
  return ss.str();//return a string with the contents of the stream
}

/*
 * returns string of functions name 
 */
string MigrationOutliner::getFunctionName(SgFunctionDeclaration *func){
        const SgName name = func->get_name();
        return name.getString();
}//end getFunctionName

void copyToFunctionBody(SgStatement* def, SgFunctionDeclaration* dest){
#define TOOL "copyBody_helper"
  DEBUG(TOOL, "\tCopying function body...");

  // Take the body of the old function and use that for the new function.
  SgTreeCopy deepCopy;
  //SgFunctionDefinition *def = isSgFunctionDefinition(src->copy(deepCopy));
  ROSE_ASSERT(def != NULL);

  setSourcePositionForTransformation(def);

  // Link body and function together
  dest->set_definition((SgFunctionDefinition*)def);
  //def->set_declaration(dest);

DEBUG(TOOL, "\tDone copying function body...");
}//end copyFunctionBody




/***************************** For-loop outline functions **************************************/
void MigrationOutliner::find_NON_OMP_ForLoops(){

}

/*
 * returns test condition of for loop
 */
string MigrationOutliner::getForloopTest(SgForStatement *forstmt){
        SgStatement* testcondition = forstmt->get_test();
        return testcondition->unparseToString();
}//end getFunctionName


void MigrationOutliner::outline_ForToFunction(SgForStatement *f){
	#define TOOL "OutlineFor_ToFunction"
	string msg;
	msg = "START: outline For loops";
	DEBUG(TOOL,msg);

	//get parent function to create appropriate Name
	
	//append name to reflect position
	string addon = convertInt(counter);
	string outlineFuncName = "foorLOOP";
	outlineFuncName +=addon;
	++counter;
	//make parameterList
	SgFunctionParameterList *pList = new SgFunctionParameterList();
	
	//make the func
	SgFunctionDeclaration *outlined = buildDefiningFunctionDeclaration(outlineFuncName, buildVoidType(), pList, f->get_scope());
	//query for dependencies


//make function declaration copy at top?
//SgFunctionDeclaration* outlineFunc_Decl = buildNondefiningFunctionDeclaration(outlineFuncName, buildVoidType(), pList, f->get_scope());

//fill function with guts
SgStatement* loop_body = f->get_loop_body();
//SgBasicBlock* lbody = loop_body->get_definition();
ROSE_ASSERT(loop_body != NULL);
copyToFunctionBody(loop_body,outlined);
appendStatement(outlined, f->get_scope());
//add dependencies to function parameters

//insert funciton declaration in same file as where for loop originates

//insert function definition at EOF

}





/*************************** OMP region outline functions **************************************/
void MigrationOutliner::outline_OMPToFunction(){
#define TOOL "OutlineOMP_ToFunction"

//get OMP REGION

//get parent function

//append name to reflect position

//create function

//query for dependencies

//add dependencies to function parameters

//insert funciton declaration in same file as where for loop originates

//insert function definition at EOF


}
