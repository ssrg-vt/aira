/*
 * mpi_call_builder.cpp
 *
 *  Created on: Feb 8, 2013
 *      Author: rlyerly
 */

#include "rose.h" // Must come first for PCH to work.
#include "mpi_call_builder.h"
#include "partitioning_constants.h"

using namespace SageBuilder;
using namespace SageInterface;

/*
 * A string jump table tied to an enum to access MPI function names.  Use the
 * macro "MPI_FUNC" to get a character string corresponding to the MPI function:
 *
 * MPI_FUNC(mpi_func_select.MPI_XXX);
 */
#define MPI_FUNC( func ) mpi_functions[func]

const char* mpi_functions[] = {
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
};

enum mpi_function_select {
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
	MPI_SENDRECV,	//Blocking send/received
};

/*
 * A string jump table tied to an enum to access MPI types.  Use the macro
 * "MPI_TYPE" to get a character string corresponding to the MPI type:
 *
 * MPI_TYPE(mpi_type_select.MPI_XXX);
 */

#define MPI_TYPE( type ) mpi_types[type]

const char* mpi_types[] = {
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
	"MPI_PACKED",
};

enum mpi_type_select {
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
	MPI_PACKED,
};

/*
 * Default constructor
 */
MPICallBuilder::MPICallBuilder() {
	//No-op for now...
}

/*
 * Builds an MPI send function call, which sends the data specified in buffer
 */
SgExprStatement* MPICallBuilder::buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		int partition, SgInitializedName* size)
{
	SgName funcName(MPI_FUNC(MPI_SEND));
	std::vector<SgExpression*> funcArgs;

	//Build a reference to the buffer and discover its size (if its an array
	//or pointer)
	SgExpression* varExp;
	if(isSgArrayType(buffer->get_type()) || isSgPointerType(buffer->get_type()))
	{
		ROSE_ASSERT(size != NULL);
		varExp = buildVarRefExp(buffer, scope);
		SgVarRefExp* sizeRef = buildVarRefExp(size, scope);
		funcArgs.push_back(varExp);
		funcArgs.push_back(sizeRef);
	}
	else
	{
		varExp = buildAddressOfOp(buildVarRefExp(buffer, scope));
		SgIntVal* sizeExp = buildIntVal(1);
		funcArgs.push_back(varExp);
		funcArgs.push_back(sizeExp);
	}
	SgVarRefExp* typeExp = buildVarRefExp(getType(buffer->get_type()), scope);
	funcArgs.push_back(typeExp);
	SgIntVal* rankExp = buildIntVal(partition);
	funcArgs.push_back(rankExp);
	SgIntVal* tagExp = buildIntVal(1);
	funcArgs.push_back(tagExp); //TODO do we want tags?  No for now...
	SgVarRefExp* commExp = buildVarRefExp(COMM_WORLD, getGlobalScope(scope));
	funcArgs.push_back(commExp);

	SgExprStatement* newFuncCall = buildFunctionCallStmt(funcName, buildVoidType(), buildExprListExp(funcArgs),	scope);

	return newFuncCall;
}

/*
 * Builds an MPI receive call, which stores data in the specified buffer
 */
SgExprStatement* MPICallBuilder::buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		int partition, SgInitializedName* status, SgInitializedName* size)
{
	SgName funcName(MPI_FUNC(MPI_RECV));
	std::vector<SgExpression*> funcArgs;

	SgExpression* varExp;
	if(isSgArrayType(buffer->get_type()) || isSgPointerType(buffer->get_type()))
	{
		ROSE_ASSERT(size != NULL);
		varExp = buildVarRefExp(buffer, scope);
		SgVarRefExp* sizeRef = buildVarRefExp(size, scope);
		funcArgs.push_back(varExp);
		funcArgs.push_back(sizeRef);
	}
	else
	{
		varExp = buildAddressOfOp(buildVarRefExp(buffer, scope));
		SgIntVal* sizeExp = buildIntVal(1);
		funcArgs.push_back(varExp);
		funcArgs.push_back(sizeExp);
	}
	SgVarRefExp* typeExp = buildVarRefExp(getType(buffer->get_type()), scope);
	funcArgs.push_back(typeExp);
	SgIntVal* rankExp = buildIntVal(partition);
	funcArgs.push_back(rankExp);
	SgIntVal* tagExp = buildIntVal(1);
	funcArgs.push_back(tagExp); //TODO do we want tags?  No for now...
	SgVarRefExp* commExp = buildVarRefExp(COMM_WORLD, getGlobalScope(scope));
	funcArgs.push_back(commExp);
	SgExpression* statusExp = buildAddressOfOp(buildVarRefExp(status, scope));
	funcArgs.push_back(statusExp);

	SgExprStatement* newFuncCall = buildFunctionCallStmt(funcName, buildVoidType(), buildExprListExp(funcArgs), scope);
	return newFuncCall;
}

/*
 * Helper function to convert an SgType node into a string representing the
 * equivalent MPI type.
 */
const char* MPICallBuilder::getType(SgType* type) {
	if(isSgTypeChar(type))
		return MPI_TYPE(MPI_CHAR);
	if(isSgTypeWchar(type))
		return MPI_TYPE(MPI_WCHAR);
	if(isSgTypeShort(type))
		return MPI_TYPE(MPI_SHORT);
	else if(isSgTypeInt(type) || isSgTypeSignedInt(type))
		return MPI_TYPE(MPI_INT);
	else if(isSgTypeLong(type))
		return MPI_TYPE(MPI_LONG);
	else if(isSgTypeLongLong(type))
		return MPI_TYPE(MPI_LONG_LONG);
	else if(isSgTypeSignedChar(type))
		return MPI_TYPE(MPI_SIGNED_CHAR);
	else if(isSgTypeUnsignedChar(type))
		return MPI_TYPE(MPI_UNSIGNED_CHAR);
	else if(isSgTypeUnsignedShort(type))
		return MPI_TYPE(MPI_UNSIGNED_SHORT);
	else if(isSgTypeUnsignedInt(type))
		return MPI_TYPE(MPI_UNSIGNED);
	else if(isSgTypeUnsignedLong(type))
		return MPI_TYPE(MPI_UNSIGNED_LONG);
	else if(isSgTypeUnsignedLongLong(type))
		return MPI_TYPE(MPI_UNSIGNED_LONG_LONG);
	else if(isSgTypeFloat(type))
		return MPI_TYPE(MPI_FLOAT);
	else if(isSgTypeDouble(type))
		return MPI_TYPE(MPI_DOUBLE);
	else if(isSgTypeLongDouble(type))
		return MPI_TYPE(MPI_LONG_DOUBLE);
	else if(isSgTypeComplex(type))
		return MPI_TYPE(MPI_C_COMPLEX);
	else if(isSgPointerType(type))
		return getType(isSgPointerType(type)->get_base_type());
	else if(isSgArrayType(type))
		return getType(isSgArrayType(type)->get_base_type());
	return NULL;
}
