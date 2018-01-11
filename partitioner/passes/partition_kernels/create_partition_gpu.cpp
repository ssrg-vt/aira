/*
 * create_partition_gpu.cpp
 *
 *  Created on: Apr 26, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "partition_kernels_common.h"
#include "create_partition.h"

/*
 * Static member definitions.
 */
bool GPUPartition::correctedHeader = false;

/*
 * Note: This will break if calling 'partition' from an arbitrary location.
 * This is okay for now, because it works with the driver script.
 */
const char* emptyGPUTemplate = "./passes/partition_kernels/templates/empty_partition_gpu.c";
const char* correctedGPUTemplate =
		"./passes/partition_kernels/templates/corrected_empty_partition_gpu.c";

/*
 * Construct/initialize a GPU partition.
 */
GPUPartition::GPUPartition(const string &p_name, const string& outfileName) :
	Partition(p_name, emptyGPUTemplate, outfileName),
	errCode(NULL)
{
	//Find error code variable
	SgName errCodeName("__err_code");
	SgVariableSymbol* errCodeSymbol = lookupVariableSymbolInParentScopes(errCodeName,
			getFirstGlobalScope(project));
	ROSE_ASSERT(errCodeSymbol != NULL);
	errCode = errCodeSymbol->get_declaration()->get_declaration();
	ROSE_ASSERT(errCode != NULL);

	currentFuncNum = 0;
	string msg = "Created GPU partition " + *name;
	DEBUG(TOOL, msg);
}

/*
 * Unparse the GPU partition AST to file and cleanup.
 */
GPUPartition::~GPUPartition()
{
	string msg = "Destroyed GPU partition " + *name;
	DEBUG(TOOL, msg);
}

/*
 * Return the partition type (GPU).
 */
enum Hardware GPUPartition::getPartitionType()
{
	return GPU;
}

/*
 * Move a kernel entry point (denoted by a function declaration) to the GPU
 * partition.
 */
Partition::FCode GPUPartition::moveFunction(SgFunctionDeclaration* func)
{
	ROSE_ASSERT(func);

	string msg = "Moving function " + NAME(func) + " into partition " + *name;
	DEBUG(TOOL, msg);

	SgFunctionDeclaration* newFunc = createEmptyFunction(func, false);
	copyFunctionBody(func, newFunc);
	setParameterList(func, newFunc);

	currentFuncNum++;
	funcs.insert(pair<FCode, SgFunctionDeclaration*>(currentFuncNum, newFunc));
	return currentFuncNum;
}

/*
 * Move a function called by a kernel (i.e. not a kernel entry point) into
 * the GPU partition.
 */
Partition::FCode GPUPartition::moveSupportingFunction(SgFunctionDeclaration* func)
{
	ROSE_ASSERT(func);

	string msg = "Moving supporting function " + NAME(func) + " into partition "
		+ *name;
	DEBUG(TOOL, msg);

	SgFunctionDeclaration* newFunc = createEmptyFunction(func, true);
	copyFunctionBody(func, newFunc);
	setParameterList(func, newFunc);

	//Note: we do not currently make this function externally available after
	//it has been moved (i.e. we don't add it to the map).

	return currentFuncNum;
}


/*
 * Internal method
 *
 * Create an empty function, which constitutes the kernel entry point on the
 * GPU partition.
 */
SgFunctionDeclaration* GPUPartition::createEmptyFunction(SgFunctionDeclaration* func,
		bool isSupportingFunc)
{
	DEBUG(TOOL, "\tCreating function declaration...");

	string kernelName = NAME(func);
	if(kernelName.find("_x86") != string::npos)
		kernelName = kernelName.substr(0, kernelName.length() - 4);
	SgName funcName(kernelName);

	//If supporting function, keep same name, otherwise, append partition name
	if(!isSupportingFunc)
		funcName = funcName + "_" + *name;

	//Get template file scope
	SgGlobal* scope = templateFile->get_globalScope();
	ROSE_ASSERT(scope);
	ROSE_ASSERT(scope->supportsDefiningFunctionDeclaration());

	//Create empty function declaration
	SgFunctionDeclaration* newFunc = buildDefiningFunctionDeclaration(funcName,
			func->get_orig_return_type(), buildFunctionParameterList(), scope);

	//Insert into source
	insertStatement(errCode, newFunc, false);
	return newFunc;
}

/*
 * Internal method
 *
 * Copy the function body from the source to the destination.  Used to copy the
 * kernel source code into the partition.
 */
void GPUPartition::copyFunctionBody(SgFunctionDeclaration* src, SgFunctionDeclaration* dest)
{
	DEBUG(TOOL, "\tCopying function body...");

	//Deep copy body of the old function
	SgTreeCopy deepCopy;
	SgFunctionDefinition* def = isSgFunctionDefinition(src->get_definition()->copy(deepCopy));
	ROSE_ASSERT(def);
	setSourcePositionForTransformation(def);

	//Link body and function together
	dest->set_definition(def);
	def->set_declaration(dest);
}

/*
 * Internal function
 *
 * Copy the function parameter list from the source to the destination.  This
 * is used to mimic the kernel interface on the GPU partition.
 */
void GPUPartition::setParameterList(SgFunctionDeclaration* src, SgFunctionDeclaration* dest)
{
	DEBUG(TOOL, "\tSetting parameter list...");

	//Copy arguments from original function
	//TODO something really strange is going on w/ the iterator...it segfaults
	//unless it is initialized
	SgInitializedNamePtrList& oldArgs = src->get_args();
	SgInitializedNamePtrList::const_iterator argIt = oldArgs.begin();
	SgInitializedName* arg = NULL;
	SgFunctionParameterList* newArgs = dest->get_parameterList();
	for(argIt == oldArgs.begin(); argIt != oldArgs.end(); argIt++)
	{
		arg = buildInitializedName(NAME(*argIt), (*argIt)->get_type());
		arg->set_scope(dest->get_definition()->get_body());
		newArgs->append_arg(arg);
	}
}

/*
 * Perform any steps needed to finalize a function.
 */
void GPUPartition::finalizeFunction(FCode funcNum)
{
	// no-op for now
}

/*
 * Add the specified global variable (as an extern variable) to the partition
 * if it is not already there.
 */
void GPUPartition::addGlobalVar(SgInitializedName* var)
{
	if(globalVars.find(var) == globalVars.end())
	{
		globalVars.insert(var);
		string msg = "\tAdding global variable " + NAME(var) + " to " + *name;
		DEBUG(TOOL, msg);

		//Create an extern version of the variable declaration
		SgVariableDeclaration* globalVar = buildVariableDeclaration(var->get_name(),
				var->get_type(), var->get_initializer(), getFirstGlobalScope(project));
		setStatic(globalVar); //TODO do we need this?  May help avoid conflicts...
		insertStatement(errCode, globalVar, false);
	}
}

/*
 * Add a class to the GPU partition.
 */
void GPUPartition::addClassDeclaration(SgClassType* type)
{
	//Copy class declaration into partition
	if(classes.find(type) == classes.end())
	{
		string msg = "\tAdding class declaration for " + type->get_name() + " to " + *name;
		DEBUG(TOOL, msg);

		SgTreeCopy deepCopy;
		SgClassDeclaration* classDecl =
				isSgClassDeclaration(
						type->get_declaration()->get_definingDeclaration()->copy(deepCopy));
		insertStatement(errCode, classDecl);
		setSourcePositionForTransformation(classDecl);
		classes.insert(type);
	}
}

/*
 * Set the GPU interface header in the partition source.
 */
void GPUPartition::setGPUHeader(const string& gpuHeader)
{
	string comment = "// @@GPU_INCLUDE_HERE@@";
	setHeader(gpuHeader, comment, emptyGPUTemplate, correctedGPUTemplate);
	if(!correctedHeader)
	{
		emptyGPUTemplate = correctedGPUTemplate;
		correctedHeader = true;
	}
}

/*
 * Set the GPU error interface header (which defines cudaError_t) in the
 * partition source.
 */
void GPUPartition::setGPUErrorHeader(const string& gpuHeader)
{
	string comment = "// @@GPU_ERROR_INCLUDE_HERE@@";
	setHeader(gpuHeader, comment, emptyGPUTemplate, correctedGPUTemplate);
	if(!correctedHeader)
	{
		emptyGPUTemplate = correctedGPUTemplate;
		correctedHeader = true;
	}
}
