/*
 * mpi_call_builder.h
 *
 *  Created on: May 9, 2013
 *      Author: rlyerly
 */

#ifndef MPI_CALL_BUILDER_H_
#define MPI_CALL_BUILDER_H_

namespace MPICallBuilder {

SgExprStatement* buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* size = NULL);
SgExprStatement* buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* size);
SgExprStatement* buildMPISend(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* type, SgExpression* size);
SgExprStatement* buildMPISend(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* size);
SgExprStatement* buildMPISend(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgExpression* type, SgExpression* size);

SgExprStatement* buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgInitializedName* size = NULL);
SgExprStatement* buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* size);
SgExprStatement* buildMPIReceive(SgInitializedName* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* type,
		SgExpression* size);
SgExprStatement* buildMPIReceive(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* size);
SgExprStatement* buildMPIReceive(SgExpression* buffer, SgScopeStatement* scope,
		enum Hardware partition, SgInitializedName* status, SgExpression* type,
		SgExpression* size);

vector<SgStatement*> buildMPIDatatypeDecl(SgClassType* type, SgScopeStatement* scope,
	SgVariableDeclaration* datatype);

void instrumentMainForMPI(SgFunctionDeclaration* main, enum Hardware partition);
SgExprStatement* buildMPIInit(SgScopeStatement* scope);
SgExprStatement* buildMPIFinalize(SgScopeStatement* scope);

} /* namespace MPICallBuilder */
#endif /* MPI_CALL_BUILDER_H_ */
