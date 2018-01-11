#include "rose.h"

#include "pragma_handling.h"

using namespace SageInterface;

Pragmas::Pragmas(SgFunctionDeclaration* p_function) :
	function(p_function),
	firstPragma(NULL),
	archsPragma(NULL),
	compatibleArchsPragma(NULL),
	inputsPragma(NULL),
	globalInputsPragma(NULL),
	outputsPragma(NULL),
	globalOutputsPragma(NULL),
	funcsNeededPragma(NULL),
	classesNeededPragma(NULL),
	partitionedPragma(NULL)
{
	ROSE_ASSERT(function);

	//Walk backwards from declaration to find pragmas
	SgStatement* stmt = getPreviousStatement(function);
	SgPragmaDeclaration* pragma = isSgPragmaDeclaration(stmt);
	PragmaParser* pp = NULL;
	while(pragma)
	{
		pp = new PragmaParser(pragma);
		if(pp->isPopcornPragma())
		{
			firstPragma = pragma;
			switch(pp->getPragmaType())
			{
			case ARCH:
				archsPragma = pp;
				break;
			case COMPATIBLE_ARCH:
				compatibleArchsPragma = pp;
				break;
			case INPUT:
				inputsPragma = pp;
				break;
			case GLOBAL_INPUT:
				globalInputsPragma = pp;
				break;
			case OUTPUT:
				outputsPragma = pp;
				break;
			case GLOBAL_OUTPUT:
				globalOutputsPragma = pp;
				break;
			case FUNCS_NEEDED:
				funcsNeededPragma = pp;
				break;
			case CLASSES_NEEDED:
				classesNeededPragma = pp;
				break;
			case PARTITIONED:
				partitionedPragma = pp;
				break;
			default:
				//TODO error message?
				break;
			}
		}
		else
			delete pp;

		stmt = getPreviousStatement(stmt);
		pragma = isSgPragmaDeclaration(stmt);
	}
}

Pragmas::~Pragmas()
{
	if(archsPragma)
		delete archsPragma;
	if(compatibleArchsPragma)
		delete compatibleArchsPragma;
	if(inputsPragma)
		delete inputsPragma;
	if(globalInputsPragma)
		delete globalInputsPragma;
	if(outputsPragma)
		delete outputsPragma;
	if(globalOutputsPragma)
		delete globalOutputsPragma;
	if(funcsNeededPragma)
		delete funcsNeededPragma;
	if(classesNeededPragma)
		delete classesNeededPragma;
	if(partitionedPragma)
		delete partitionedPragma;
}

SgStatement* Pragmas::getFirstPragma()
{
	return firstPragma;
}

PragmaParser* Pragmas::getArchs()
{
	return archsPragma;
}

PragmaParser* Pragmas::getCompatibleArchs()
{
	return compatibleArchsPragma;
}

PragmaParser* Pragmas::getInputs()
{
	return inputsPragma;
}

PragmaParser* Pragmas::getGlobalInputs()
{
	return globalInputsPragma;
}

PragmaParser* Pragmas::getOutputs()
{
	return outputsPragma;
}

PragmaParser* Pragmas::getGlobalOutputs()
{
	return globalOutputsPragma;
}

PragmaParser* Pragmas::getFuncsNeeded()
{
	return funcsNeededPragma;
}

PragmaParser* Pragmas::getClassesNeeded()
{
	return classesNeededPragma;
}

PragmaParser* Pragmas::getPartitioned()
{
	return partitionedPragma;
}

const SgFunctionDeclaration* Pragmas::getFunction()
{
	return function;
}

void Pragmas::removePragmas()
{
	if(archsPragma)
		removeStatement(archsPragma->getPragmaDeclaration());
	if(compatibleArchsPragma)
		removeStatement(compatibleArchsPragma->getPragmaDeclaration());
	if(inputsPragma)
		removeStatement(inputsPragma->getPragmaDeclaration());
	if(globalInputsPragma)
		removeStatement(globalInputsPragma->getPragmaDeclaration());
	if(outputsPragma)
		removeStatement(outputsPragma->getPragmaDeclaration());
	if(globalOutputsPragma)
		removeStatement(globalOutputsPragma->getPragmaDeclaration());
	if(funcsNeededPragma)
		removeStatement(funcsNeededPragma->getPragmaDeclaration());
	if(classesNeededPragma)
		removeStatement(classesNeededPragma->getPragmaDeclaration());
	if(partitionedPragma)
		removeStatement(partitionedPragma->getPragmaDeclaration());
}

