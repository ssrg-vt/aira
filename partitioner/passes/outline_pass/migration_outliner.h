/* migration_outliner.h
 * 
 * Header for migration_outliner
 * Created: 10/22/2014
 * Author: bielsk1
 */

#ifndef MIGRATION_OUTLINER_H_
#define MIGRATION_OUTLINER_H_
using namespace SageInterface;
using namespace SageBuilder;
#include <string>
#include <vector>

class MigrationOutliner{
public:
	MigrationOutliner(SgProject* p);

        string getFunctionName(SgFunctionDeclaration* f);
	string getForloopTest(SgForStatement *f);
	void outline_ForToFunction(SgForStatement *f);
	void outline_OMPToFunction();
	void find_NON_OMP_ForLoops();

/*        void setPopcornScope(SgFunctionDeclaration* f);
        void setMigrationScope(SgFunctionDeclaration* f);
        SgFunctionParameterList* getParameters(SgFunctionDeclaration* f);
        SgClassDeclaration* makeStruct4Func(SgFunctionDeclaration* f);
        void removeOrigFunc(SgFunctionDeclaration* f);
        void makePopcornFunc(SgFunctionDeclaration* f,SgClassDeclaration* s);
        void makeMigrationHint(SgFunctionDeclaration* f);
        SgFunctionDeclaration* makePickKernel();
        void insertStrucs4File(SgClassDeclaration* s, SgFunctionDeclaration* f, int uniqueNum);
        void insertDependencies(SgProject* project);
        void mainMigrateTransform(SgProject* project);
        void rollCall_Popcorn();
        void insertPopcornFuncs(SgFunctionDeclaration* f);
        void make_x86File();
        void insert_hint();
        void insertTYPE_H();    
        void copyThreadprivate_Pragmas();
        void solveMoreDependency(SgFunctionDeclaration* f);
        void addPopcornGlblVarRefsToArray(SgScopeStatement* currScope);
        void clearPopcornGlblVarArray();
        void replaceMathWithPopcornLIBM(SgFunctionCallExp*);
*/

private:
	SgFunctionDeclaration* main_body;	
	SgProject* project;
	int counter;
};


#endif /* MIGRATION_OUTLINER_H */
