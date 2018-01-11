/*
 * mpi_call_builder.cpp
 *
 *  Created on: May 9, 2013
 *      Author: rlyerly
 */

#include "rose.h"
#include "common.h"
#include "mpi_call_builder.h"

#define TOOL "MPI Call Builder"

using namespace std;
using namespace SageBuilder;
using namespace SageInterface;

#define COMM_WORLD "MPI_COMM_WORLD"

/*
 * A string jump table tied to an enum to access MPI function names.  Use the
 * macro "MPI_FUNC" to get a character string corresponding to the MPI function:
 *
 * MPIFUNC(mpi_func_select.MPI_XXX);
 *
 * TODO this is some real C-style nastiness...is there a better way to do this?
 */
#define MPIFUNC( func ) mpiFunctions[func]

static const char* mpiFunctions[] = {
	"MPI_Send",
	"MPI_Ssend",
	"MPI_Bsend",
	"MPI_Rsend",
	"MPI_Isend",
	"MPI_Issend",
	"MPI_Ibsend",
	"MPI_Irsend",
	"MPI_Recv",
	"MPI_Irecv",
	"MPI_Sendrecv",
	"MPI_Comm_size",
	"MPI_Comm_rank"
};

enum mpiFunctionSelect {
	MPI_SEND,		//Blocking send
	MPI_SSEND,		//Synchronous blocking send
	MPI_BSEND,		//Buffered blocking send
	MPI_RSEND,		//Blocking ready send
	MPI_ISEND,		//Non-blocking send
	MPI_ISSEND,		//Non-blocking synchronous send
	MPI_IBSEND,		//Non-blocking buffered send
	MPI_IRSEND,		//Non-blocking ready send
	MPI_RECV,		//Blocking receive
	MPI_IRECV,		//Non-blocking receive
	MPI_SENDRECV,		//Blocking send/received
	MPI_COMM_SIZE,		//Get number of processes in the communicator
	MPI_COMM_RANK		//Get current process' rank
};

/*
 * A string jump table tied to an enum to access MPI types.  Use the macro
 * "MPITYPE" to get a character string corresponding to the MPI type:
 *
 * MPITYPE(mpiTypeSelect.MPI_XXX);
 *
 * TODO again, nasty C-style stuff
 */
#define MPITYPE( type ) mpiTypes[type]

static const char* mpiTypes[] = {
	"MPI_CHAR",
	"MPI_WCHAR",
	"MPI_SHORT",
	"MPI_INT",
	"MPI_LONG",
	"MPI_LONG_LONG_INT",
	"MPI_LONG_LONG",
	"MPI_SIGNED_CHAR",
	"MPI_UNSIGNED_CHAR",
	"MPI_UNSIGNED_SHORT",
	"MPI_UNSIGNED",
	"MPI_UNSIGNED_LONG",
	"MPI_UNSIGNED_LONG_LONG",
	"MPI_FLOAT",
	"MPI_DOUBLE",
	"MPI_LONG_DOUBLE",
	"MPI_C_COMPLEX",
	"MPI_C_FLOAT_COMPLEX",
	"MPI_C_DOUBLE_COMPLEX",
	"MPI_C_LONG_DOUBLE_COMPLEX",
	"MPI_C_BOOL",
	"MPI_INT8_T",
	"MPI_INT16_T",
	"MPI_INT32_T",
	"MPI_INT64_T",
	"MPI_UINT8_T",
	"MPI_UINT16_T",
	"MPI_UINT32_T",
	"MPI_UINT64_T",
	"MPI_BYTE",
	"MPI_PACKED"
};

enum mpiTypeSelect {
 	MPI_CHAR,
 	MPI_WCHAR,
 	MPI_SHORT,
 	MPI_INT,
 	MPI_LONG,
	MPI_LONG_LONG_INT,
	MPI_LONG_LONG,
	MPI_SIGNED_CHAR,
	MPI_UNSIGNED_CHAR,
	MPI_UNSIGNED_SHORT,
	MPI_UNSIGNED,
	MPI_UNSIGNED_LONG,
	MPI_UNSIGNED_LONG_LONG,
	MPI_FLOAT,
	MPI_DOUBLE,
	MPI_LONG_DOUBLE,
	MPI_C_COMPLEX,
	MPI_C_FLOAT_COMPLEX,
	MPI_C_DOUBLE_COMPLEX,
	MPI_C_LONG_DOUBLE_COMPLEX,
	MPI_C_BOOL,
	MPI_INT8_T,
	MPI_INT16_T,
	MPI_INT32_T,
	MPI_INT64_T,
	MPI_UINT8_T,
	MPI_UINT16_T,
	MPI_UINT32_T,
	MPI_UINT64_T,
	MPI_BYTE,
	MPI_PACKED
};

/*
 * Internal helper function, should not be called outside of this file.
 */
static const char* getType(SgType* p_type)
{
	SgType* type = p_type->findBaseType();

	if(isSgTypeChar(type))
		return MPITYPE(MPI_CHAR);
	if(isSgTypeWchar(type))
		return MPITYPE(MPI_WCHAR);
	if(isSgTypeBool(type))
		return MPITYPE(MPI_C_BOOL);
	if(isSgTypeShort(type))
		return MPITYPE(MPI_SHORT);
	if(isSgTypeInt(type) || isSgTypeSignedInt(type))
		return MPITYPE(MPI_INT);
	if(isSgTypeLong(type))
		return MPITYPE(MPI_LONG);
	if(isSgTypeLongLong(type))
		return MPITYPE(MPI_LONG_LONG);
	if(isSgTypeSignedChar(type))
		return MPITYPE(MPI_SIGNED_CHAR);
	if(isSgTypeUnsignedChar(type))
		return MPITYPE(MPI_UNSIGNED_CHAR);
	if(isSgTypeUnsignedShort(type))
		return MPITYPE(MPI_UNSIGNED_SHORT);
	if(isSgTypeUnsignedInt(type))
		return MPITYPE(MPI_UNSIGNED);
	if(isSgTypeUnsignedLong(type))
		return MPITYPE(MPI_UNSIGNED_LONG);
	if(isSgTypeUnsignedLongLong(type))
		return MPITYPE(MPI_UNSIGNED_LONG_LONG);
	if(isSgTypeFloat(type))
		return MPITYPE(MPI_FLOAT);
	if(isSgTypeDouble(type))
		return MPITYPE(MPI_DOUBLE);
	if(isSgTypeLongDouble(type))
		return MPITYPE(MPI_LONG_DOUBLE);
	if(isSgTypeComplex(type))
		return MPITYPE(MPI_C_COMPLEX);
	return NULL;
}

namespace MPICallBuilder
{

/*
 * Build an MPI Send command to transfer data to a partition.
 *
 * This version of the overloaded function allows you to specify an initialized
 * name variable representing the size.
 */
SgExprStatement* buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* size)
{
	return buildMPISend(buffer, scope, partition,
		(size != NULL ? buildVarRefExp(size, scope) : NULL));
}

/*
 * Build an MPI Send command to transfer data to a partition.
 *
 * This version of the overloaded function allows you to specify an expression
 * representing the size.
 */
SgExprStatement* buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* size)
{
	SgVarRefExp* typeExp = buildVarRefExp(getType(buffer->get_type()), scope);
	return buildMPISend(buffer, scope, partition, typeExp, size);
}

/*
 * Build an MPI Send command to transfer data to a partition.
 *
 * This version of the overloaded function allows you to specify an expression
 * representing the type & size.
 */
SgExprStatement* buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* type, SgExpression* size)
{
	SgName funcName(MPIFUNC(MPI_SEND));
	vector<SgExpression*> funcArgs;

	SgExpression* varExp;
	SgUnsignedLongVal* sizeExp;
	if(isSgArrayType(buffer->get_type()) || isSgPointerType(buffer->get_type()))
	{
		ROSE_ASSERT(size);
		varExp = buildVarRefExp(buffer, scope);
		funcArgs.push_back(varExp);
		funcArgs.push_back(size);
	}
	else
	{
		varExp = buildAddressOfOp(buildVarRefExp(buffer, scope));
		sizeExp = buildUnsignedLongVal(1);
		funcArgs.push_back(varExp);
		funcArgs.push_back(sizeExp);
	}
	funcArgs.push_back(type);
	SgIntVal* rankExp = buildIntVal(partition);
	funcArgs.push_back(rankExp);
	SgIntVal* tagExp = buildIntVal(1); //TODO do we want tags?  No for now...
	funcArgs.push_back(tagExp);
	SgVarRefExp* commExp = buildVarRefExp(COMM_WORLD, getGlobalScope(scope));
	funcArgs.push_back(commExp);

	return buildFunctionCallStmt(funcName, buildVoidType(), buildExprListExp(funcArgs), scope);
}

/*
 * Build an MPI Send command to transfer data to a partition.
 *
 * This version of the overloaded function allows you to specify an expression
 * for the buffer.
 */
SgExprStatement* buildMPISend(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* size)
{
	SgVarRefExp* typeExp = buildVarRefExp(getType(buffer->get_type()), scope);
	return buildMPISend(buffer, scope, partition, typeExp, size);
}

SgExprStatement* buildMPISend(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* type, SgExpression* size)
{
	SgName funcName(MPIFUNC(MPI_SEND));
	vector<SgExpression*> funcArgs;

	funcArgs.push_back(buffer);
	SgUnsignedLongVal* sizeExp;
	if(isSgArrayType(buffer->get_type()) || isSgPointerType(buffer->get_type()))
	{
		ROSE_ASSERT(size);
		funcArgs.push_back(size);
	}
	else
	{
		sizeExp = buildUnsignedLongVal(1);
		funcArgs.push_back(sizeExp);
	}
	funcArgs.push_back(type);
	SgIntVal* rankExp = buildIntVal(partition);
	funcArgs.push_back(rankExp);
	SgIntVal* tagExp = buildIntVal(1); //TODO do we want tags?  No for now...
	funcArgs.push_back(tagExp);
	SgVarRefExp* commExp = buildVarRefExp(COMM_WORLD, getGlobalScope(scope));
	funcArgs.push_back(commExp);

	return buildFunctionCallStmt(funcName, buildVoidType(), buildExprListExp(funcArgs), scope);
}

/*
 * Build an MPI Receive command to transfer data from a partition.
 *
 * This version of the overloaded function allows you to specify an initialized
 * name variable representing the size.
 */
SgExprStatement* buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgInitializedName* size)
{
	return buildMPIReceive(buffer, scope, partition, status,
		(size != NULL ? buildVarRefExp(size, scope) : NULL));
}

/*
 * Build an MPI Receive command to transfer data from a partition.
 *
 * This version of the overloaded function allows you to specify an expression
 * representing the size.
 */
SgExprStatement* buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* size)
{
	SgVarRefExp* typeExp = buildVarRefExp(getType(buffer->get_type()), scope);
	return buildMPIReceive(buffer, scope, partition, status, typeExp, size);
}

/*
 * Build an MPI Receive command to transfer data from a partition.
 *
 * This version of the overloaded function allows you to specify an expression
 * representing the type & size.
 */
SgExprStatement* buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* type,
		SgExpression* size)
{
	SgName funcName(MPIFUNC(MPI_RECV));
	vector<SgExpression*> funcArgs;

	SgExpression* varExp;
	SgUnsignedLongVal* sizeExp;
	if(isSgArrayType(buffer->get_type()) || isSgPointerType(buffer->get_type()))
	{
		ROSE_ASSERT(size);
		varExp = buildVarRefExp(buffer, scope);
		funcArgs.push_back(varExp);
		funcArgs.push_back(size);
	}
	else
	{
		varExp = buildAddressOfOp(buildVarRefExp(buffer, scope));
		sizeExp = buildUnsignedLongVal(1);
		funcArgs.push_back(varExp);
		funcArgs.push_back(sizeExp);
	}
	funcArgs.push_back(type);
	SgIntVal* rankExp = buildIntVal(partition);
	funcArgs.push_back(rankExp);
	SgIntVal* tagExp = buildIntVal(1); //TODO do we want tags?  No for now...
	funcArgs.push_back(tagExp);
	SgVarRefExp* commExp = buildVarRefExp(COMM_WORLD, getGlobalScope(scope));
	funcArgs.push_back(commExp);
	SgExpression* statusExp = buildAddressOfOp(buildVarRefExp(status, scope));
	funcArgs.push_back(statusExp);

	return buildFunctionCallStmt(funcName, buildVoidType(), buildExprListExp(funcArgs), scope);
}

/*
 * Build an MPI Receive command to transfer data from a partition.
 *
 * This version of the overloaded function allows you to specify an expression
 * representing the buffer.
 */
SgExprStatement* buildMPIReceive(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* size)
{
	SgVarRefExp* typeExp = buildVarRefExp(getType(buffer->get_type()), scope);
	return buildMPIReceive(buffer, scope, partition, status, typeExp, size);
}

SgExprStatement* buildMPIReceive(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* type,
		SgExpression* size)
{
	SgName funcName(MPIFUNC(MPI_RECV));
	vector<SgExpression*> funcArgs;

	funcArgs.push_back(buffer);
	funcArgs.push_back(size);
	/*SgUnsignedLongVal* sizeExp;
	if(isSgArrayType(buffer->get_type()) || isSgPointerType(buffer->get_type()))
	{
		ROSE_ASSERT(size);
		funcArgs.push_back(size);
	}
	else
	{
		sizeExp = buildUnsignedLongVal(1);
		funcArgs.push_back(sizeExp);
	}*/
	funcArgs.push_back(type);
	SgIntVal* rankExp = buildIntVal(partition);
	funcArgs.push_back(rankExp);
	SgIntVal* tagExp = buildIntVal(1); //TODO do we want tags?  No for now...
	funcArgs.push_back(tagExp);
	SgVarRefExp* commExp = buildVarRefExp(COMM_WORLD, getGlobalScope(scope));
	funcArgs.push_back(commExp);
	SgExpression* statusExp = buildAddressOfOp(buildVarRefExp(status, scope));
	funcArgs.push_back(statusExp);

	return buildFunctionCallStmt(funcName, buildVoidType(), buildExprListExp(funcArgs), scope);
}

/*
 * Return the statements necessary to build a custom MPI datatype to transfer
 * structs/classes via MPI.
 */
vector<SgStatement*> buildMPIDatatypeDecl(SgClassType* type, SgScopeStatement* scope,
	SgVariableDeclaration* datatype)
{
	string msg;
	vector<SgStatement*> stmts;

	//Get class information
	SgClassDeclaration* classDecl =
		isSgClassDeclaration(isSgClassType(type)->get_declaration()->get_definingDeclaration());
	ROSE_ASSERT(classDecl != NULL);
	SgClassDefinition* classDef = classDecl->get_definition();
	SgClassSymbol* classSymbol =
		lookupClassSymbolInParentScopes(classDecl->get_name(), classDecl->get_scope());
	ROSE_ASSERT(classSymbol != NULL);

	SgDeclarationStatementPtrList& members = classDef->get_members();
	SgDeclarationStatementPtrList::const_iterator memberIt = members.begin();
	SgVariableDeclaration* varDecl = NULL;
	SgVarRefExp* memberRef = NULL;
	SgType* curType = NULL;
	SgExprListExp* args = NULL;

	//Define arrays of types & block lengths
	//NOTE: For now, we're not going to try to combine similar types.  Each
	//member is a separate block
	vector<SgExpression*> types;
	vector<SgExpression*> sizes;
	vector<SgExpression*> extents;
	for(memberIt = members.begin(); memberIt != members.end(); memberIt++)
	{
		varDecl = isSgVariableDeclaration(*memberIt);
		if(varDecl)
		{
			SgInitializedNamePtrList& vars = varDecl->get_variables();
			SgInitializedNamePtrList::const_iterator varIt = vars.begin();
			for(varIt = vars.begin(); varIt != vars.end(); varIt++)
			{
				msg = "\t\t\tSetting up info for " + (*varIt)->get_name();
				DEBUG(TOOL, msg);

				curType = (*varIt)->get_type();

				//Type
				//Hacky for now - no good way to simply put text in
				types.push_back(buildOpaqueVarRefExp(getType(curType), getGlobalScope(scope)));

				//Blocksize
				if(isSgArrayType(curType))
					sizes.push_back(buildIntVal(getArrayElementCount(isSgArrayType(curType))));
				else
					sizes.push_back(buildIntVal(1));

				//Extents
				//Again, hacky - have to insert arbitrary text
				//for classname
				memberRef = buildOpaqueVarRefExp((*varIt)->get_name(), getGlobalScope(scope));
				attachArbitraryText(memberRef, classSymbol->get_name().getString() + ", ");
				args = buildExprListExp(memberRef);
				extents.push_back(buildFunctionCallExp("offsetof",
					buildUnsignedLongType(), args, getGlobalScope(scope)));
			}
		}
	}

	//Build types array
	SgAggregateInitializer* typeInit = buildAggregateInitializer(buildExprListExp(types),
		buildOpaqueType("MPI_Datatype", getGlobalScope(scope)));
	SgType* datatypeArr = buildArrayType(buildOpaqueType("MPI_Datatype",
		getGlobalScope(scope)), buildIntVal(types.size()));
	SgVariableDeclaration* typesVar = buildVariableDeclaration(type->get_name() + "__types",
		datatypeArr, typeInit, scope);
	stmts.push_back(typesVar);

	//Build sizes array
	SgAggregateInitializer* sizeInit = buildAggregateInitializer(buildExprListExp(sizes), buildIntType());
	SgType* intArr = buildArrayType(buildIntType(), buildIntVal(sizes.size()));
	SgVariableDeclaration* sizesVar = buildVariableDeclaration(type->get_name() + "__sizes",
		intArr, sizeInit, scope);
	stmts.push_back(sizesVar);

	//Build extents array
	SgType* extentArr = buildArrayType(buildOpaqueType("MPI_Aint", getGlobalScope(scope)),
		buildIntVal(extents.size()));
	SgVariableDeclaration* extentsVar = buildVariableDeclaration(type->get_name() + "__extents",
		extentArr, NULL, scope);
	stmts.push_back(extentsVar);

	//Assign values to extents
	SgVarRefExp* extentsRef = buildVarRefExp(extentsVar);
	for(unsigned int i = 0; i < members.size(); i++)
	{
		SgPntrArrRefExp* arrRef = buildPntrArrRefExp(extentsRef, buildIntVal(i));
		stmts.push_back(buildAssignStatement(arrRef, extents[i]));
	}

	//Create datatype
	SgAddressOfOp* datatypeAddr = buildAddressOfOp(buildVarRefExp(datatype));
	args = buildExprListExp(buildIntVal(types.size()), buildVarRefExp(sizesVar), extentsRef,
		buildVarRefExp(typesVar), datatypeAddr);
	stmts.push_back(buildFunctionCallStmt("MPI_Type_create_struct", buildIntType(), args, scope));
	args = buildExprListExp(datatypeAddr);
	stmts.push_back(buildFunctionCallStmt("MPI_Type_commit", buildIntType(), args, scope));

	return stmts;
}

/*
 * Instrument the specified function (supposed to be main) with calls to
 * initialize and finalize the application for MPI.
 */
void instrumentMainForMPI(SgFunctionDeclaration* main, enum Hardware partition)
{
	if(!isMain(main))
	{
		string msg = "Cannot instrument function "
			+ main->get_name().getString()
			+ " because it is not main";
		DEBUG(TOOL, msg);
	}
	else
	{
		//Call "MPI_Init"
		main->get_definition()->prepend_statement(
			buildMPIInit(main->get_definition()));

		//Tell MPI partition to exit
		SgAssignInitializer* zero = buildAssignInitializer(buildIntVal(0), buildIntType());
		SgName endAppName("__finished__");
		SgVariableDeclaration* endAppVar = buildVariableDeclaration(endAppName,
			buildIntType(), zero, main->get_definition());
		instrumentEndOfFunction(main, endAppVar);

		SgName numprocName("__num_proc__");
		SgVariableDeclaration* numproc = buildVariableDeclaration(numprocName, buildIntType(),
			NULL, main->get_definition());
		instrumentEndOfFunction(main, numproc);
		SgExprListExp* args = buildExprListExp(buildOpaqueVarRefExp(COMM_WORLD, getScope(main)),
			buildAddressOfOp(buildVarRefExp(numproc)));
		SgExprStatement* numprocCall = buildFunctionCallStmt(MPIFUNC(MPI_COMM_SIZE),
			buildVoidType(), args, main->get_definition());
		instrumentEndOfFunction(main, numprocCall);

		SgStatement* check = buildExprStatement(buildEqualityOp(buildVarRefExp(numproc),
			buildIntVal(2)));
		SgInitializedName* endAppRef = buildInitializedName(endAppName, buildIntType());
		SgExprStatement* sendEndVar = buildMPISend(endAppRef, main->get_definition(),
			partition);
		instrumentEndOfFunction(main, buildIfStmt(check, sendEndVar, NULL));

		//Call "MPI_Finalize"
		instrumentEndOfFunction(main, buildMPIFinalize(main->get_definition()));

		DEBUG(TOOL, "Added calls to initialize/finalize application for MPI");
	}
}

/*
 * Build a call to MPI_Init.
 */
SgExprStatement* buildMPIInit(SgScopeStatement* scope)
{
	//TODO get actual names for these variables from argument list
	SgAddressOfOp* argc = buildAddressOfOp(buildVarRefExp("argc", scope));
	SgAddressOfOp* argv = buildAddressOfOp(buildVarRefExp("argv", scope));
	SgExprListExp* params = buildExprListExp(argc, argv);
	SgFunctionCallExp* callExp = buildFunctionCallExp("MPI_Init",
		buildIntType(), params, scope);
	return buildExprStatement(callExp);
}

/*
 * Build a call to MPI_Finalize.
 */
SgExprStatement* buildMPIFinalize(SgScopeStatement* scope)
{
	SgFunctionCallExp* callExp = buildFunctionCallExp("MPI_Finalize",
		buildIntType(), NULL, scope);
	return buildExprStatement(callExp);
}

} /* namespace MPICallBuilder */
