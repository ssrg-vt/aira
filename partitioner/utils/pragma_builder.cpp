#include "rose.h"
#include "pragma_handling.h"

/*
 * Pragma builder for variables - builds a popcorn pragma based on the
 * specified type of pragma with the names of the variables contained in the
 * specified set.
 */
SgPragmaDeclaration* PragmaBuilder::buildVariablePragma(enum pragmaType type,
		set<SgInitializedName*> vars, SgScopeStatement* scope)
{
	string curString;
	set<string> strings;
	set<SgInitializedName*>::const_iterator varIt;
	SgType* varType;
	for(varIt = vars.begin(); varIt != vars.end(); varIt++)
	{
		curString = (*varIt)->get_name().getString();
		varType = (*varIt)->get_type();
		if(isSgPointerType(varType) || isSgArrayType(varType))
			curString += "*";
		strings.insert(curString);
	}
	return buildPragma(type, strings, scope);
}

/*
 * Pragma builder for functions - builds a popcorn pragma with the names of the
 * functions contained in the specified set.
 */
SgPragmaDeclaration* PragmaBuilder::buildFunctionPragma(
		set<SgFunctionDeclaration*> funcs, SgScopeStatement* scope)
{
	set<string> strings;
	set<SgFunctionDeclaration*>::const_iterator funcIt;
	for(funcIt = funcs.begin(); funcIt != funcs.end(); funcIt++)
		strings.insert((*funcIt)->get_name().getString());
	return buildPragma(FUNCS_NEEDED, strings, scope);
}

/*
 * Pragma builder for functions - build a popcorn pragma with the names of the
 * classes contained in the specified set.
 */
SgPragmaDeclaration* PragmaBuilder::buildClassesPragma(
		set<SgClassType*> classes, SgScopeStatement* scope)
{
	set<string> strings;
	set<SgClassType*>::const_iterator classIt;
	for(classIt = classes.begin(); classIt != classes.end(); classIt++)
		strings.insert((*classIt)->get_name().getString());
	return buildPragma(CLASSES_NEEDED, strings, scope);
}

/*
 * Simple pragma builder - builds a popcorn pragma based on the specified type
 * of pragma with the strings in the specified set.
 */
SgPragmaDeclaration* PragmaBuilder::buildPragma(enum pragmaType type,
		set<string> strings, SgScopeStatement* scope)
{
	set<string>::const_iterator stringIt;
	string pragmaText(popcornKeyword + " " + type2string(type) + " ( ");
	stringIt = strings.begin();
	if(stringIt != strings.end())
	{
		pragmaText += (*stringIt);
		stringIt++;
	}
	for(; stringIt != strings.end(); stringIt++)
		pragmaText += ", " + (*stringIt);
	pragmaText += " )";
	return SageBuilder::buildPragmaDeclaration(pragmaText, scope);
}

/*
 * Converts a pragma type to a string
 */
string PragmaBuilder::type2string(enum pragmaType type)
{
	switch(type)
	{
	case ARCH:
		return "arch";
	case COMPATIBLE_ARCH:
		return "compatibleArch";
	case INPUT:
		return "inputs";
	case GLOBAL_INPUT:
		return "globalInputs";
	case OUTPUT:
		return "outputs";
	case GLOBAL_OUTPUT:
		return "globalOutputs";
	case FUNCS_NEEDED:
		return "functionsNeeded";
	case CLASSES_NEEDED:
		return "classesNeeded";
	case PARTITIONED:
		return "partitioned";
	default:
		return "unknown";
	}
}

/*
 * Converts a pragma string to a type
 */
enum pragmaType PragmaBuilder::string2type(string type)
{
	if(type == "arch")
		return ARCH;
	else if(type == "compatibleArch")
		return COMPATIBLE_ARCH;
	else if(type == "inputs")
		return INPUT;
	else if(type == "globalInputs")
		return GLOBAL_INPUT;
	else if(type == "outputs")
		return OUTPUT;
	else if(type == "globalOutputs")
		return GLOBAL_OUTPUT;
	else if(type == "functionsNeeded")
		return FUNCS_NEEDED;
	else if(type == "classesNeeded")
		return CLASSES_NEEDED;
	else if(type == "partitioned")
		return PARTITIONED;
	else
		return UNKNOWN_TYPE;
}

