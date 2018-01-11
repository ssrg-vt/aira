/* MAIN.cpp
 * Created: 04/15/2014
 * Author: bielsk1
 * Finished:	v1: 6/11/2014
 * 		v2:
 */

#include "rose.h"
#include "common.h"
//#include "preprocessor_handler.h"
#include "pragma_handling.h"
#include "migration_outliner.h"

using namespace SageBuilder;
using namespace SageInterface;
//using namespace std;
#include <iostream>
#include <string>
#define TOOL "Migration Outliner"


int main(int argc, char **argv){
string msg;
        if(SgProject::get_verbose() > 0)
                printf("In Main() \n");
        //Initialize the project
        SgProject* project = frontend(argc,argv);
      
	ROSE_ASSERT(project);
        MigrationOutliner MO(project);

project->unparse();
return 0;

/*
        //find for loops and pragmas
  Rose_STL_Container<SgNode*> pragmas = querySubTree(project, V_SgPragmaDeclaration);
  Rose_STL_Container<SgNode*> fors = querySubTree(project, V_SgForStatement);
  Rose_STL_Container<SgNode*>::const_iterator forIt;
  Rose_STL_Container<SgNode*>::const_iterator pragmaIt;
  SgForStatement* forloop;
  SgPragmaDeclaration* pragma;

  SgFunctionDeclaration* funcDecl;
  SgStatement* stmt;
  SgClassDeclaration* st;

for(forIt = fors.begin(); forIt != fors.end(); forIt++){
  forloop = isSgForStatement(*forIt);
  ROSE_ASSERT(forloop);
//  printf("statement %s",MO.getForloopTest(forloop));
  MO.outline_ForToFunction(forloop);

}//end for

for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++){
  pragma = isSgPragmaDeclaration(*pragmaIt);
  ROSE_ASSERT(pragma);

//    PragmaParser pp(pragma);

    {
  
   printf("\n\n-------------------------Next Pragma------------------------------------------------");
    //Get pragmas for function declaration
  //    stmt = getNextStatement(pragma);
    //  while(!isSgFunctionDeclaration(stmt) && isSgPragmaDeclaration(stmt))
      //  stmt = getNextStatement(stmt);
//      fu/ncDecl = isSgFunctionDeclaration(stmt);

  //    MM.setPopcornScope(funcDecl);

	



      if(!funcDecl)
      {
        msg = "Statement following popcorn pragmas is not a function declaration!";
        DEBUG(TOOL, msg);
        continue;
      }//end if

//        SgClassDeclaration* st = MM.makeStruct4Func(funcDecl);

        //make popcorn equivalent of func
//	MM.makePopcornFunc(funcDecl, st);
        
        //remove old funct
  //      MM.removeOrigFunc(funcDecl);
        
//	MM.mainMigrateTransform(project);
	//insert dependencies
	
//	MM.insertStrucs4File(st,funcDecl,uniqueness);

        //remove pragma
        //removeStatement(pragma);
  //      msg = "Annotating function: " + MM.getFunctionName(funcDecl) + " done."; DEBUG(TOOL, msg);
	
//	st = NULL;

      }//end if
   }//end for

	msg = "finished LOOP"; DEBUG(TOOL,msg);
	MM.rollCall_Popcorn();
////////////////END POPCORN TRANSFORMER- SRC BEGIN SEPERATOR ////////////////////////////////////
msg = "OUTLINE READY!"; DEBUG(TOOL,msg);


project->unparse();
return 0;
*/
//return backend(project);
}//end main
