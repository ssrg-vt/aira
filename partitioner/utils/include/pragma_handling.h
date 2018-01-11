/*
 * pragma_parser.h
 *
 * Helper classes for parsing/building pragmas.
 *
 * Created on: Apr 17, 2013
 * Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef PRAGMA_HANDLER_H_
#define PRAGMA_HANDLER_H_

#include <string>
#include <set>

using namespace std;

/*
 * Pragma keyword for Popcorn pragmas.
 */
const string popcornKeyword = "popcorn";

/*
 * Enumeration listing the types of available pragmas.
 */
enum pragmaType {
	ARCH,
	COMPATIBLE_ARCH,
	INPUT,
	GLOBAL_INPUT,
	OUTPUT,
	GLOBAL_OUTPUT,
	FUNCS_NEEDED,
	CLASSES_NEEDED,
	PARTITIONED,
	UNKNOWN_TYPE
};

/*
 * Helper class for parsing pragmas from a string.
 */
class PragmaParser {
public:
	PragmaParser(SgPragmaDeclaration* p_pragmaDeclaration);
	SgPragmaDeclaration* getPragmaDeclaration();
	bool isPopcornPragma();
	string getPragmaKeyword();
	enum pragmaType getPragmaType();
	set<string> getNames();
private:
	SgPragmaDeclaration* pragmaDeclaration;
	string pragma;
	string pragmaKeyword;
	string pragmaType;
	set<string> names;

	static string& trim(string&);
	static string& ltrim(string&);
	static string& rtrim(string&);
};

/*
 * Namespace for building pragma declarations
 */
namespace PragmaBuilder {
	SgPragmaDeclaration* buildVariablePragma(enum pragmaType,
		set<SgInitializedName*>, SgScopeStatement*);
	SgPragmaDeclaration* buildFunctionPragma(set<SgFunctionDeclaration*>,
		SgScopeStatement*);
	SgPragmaDeclaration* buildClassesPragma(set<SgClassType*>,
		SgScopeStatement*);
	SgPragmaDeclaration* buildPragma(enum pragmaType, set<string>,
		SgScopeStatement*);
	string type2string(enum pragmaType type);
	enum pragmaType string2type(string type);
};

/*
 * Wrapper class that bundles together all Popcorn pragmas for the specified
 * function for ease of use.
 */
class Pragmas {
public:
	Pragmas(SgFunctionDeclaration* p_function);
	~Pragmas();

	SgStatement* getFirstPragma();
	PragmaParser* getArchs();
	PragmaParser* getCompatibleArchs();
	PragmaParser* getInputs();
	PragmaParser* getGlobalInputs();
	PragmaParser* getOutputs();
	PragmaParser* getGlobalOutputs();
	PragmaParser* getFuncsNeeded();
	PragmaParser* getClassesNeeded();
	PragmaParser* getPartitioned();
	const SgFunctionDeclaration* getFunction();
	void removePragmas();

private:
	SgFunctionDeclaration* function;
	SgStatement* firstPragma;
	PragmaParser* archsPragma;
	PragmaParser* compatibleArchsPragma;
	PragmaParser* inputsPragma;
	PragmaParser* globalInputsPragma;
	PragmaParser* outputsPragma;
	PragmaParser* globalOutputsPragma;
	PragmaParser* funcsNeededPragma;
	PragmaParser* classesNeededPragma;
	PragmaParser* partitionedPragma;
};

#endif /* PRAGMA_HANDLER_H_ */
