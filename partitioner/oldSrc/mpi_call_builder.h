/*
 * mpi_call_builder.h
 *
 *  Created on: Feb 8, 2013
 *      Author: rlyerly
 */

#ifndef MPI_CALL_BUILDER_H_
#define MPI_CALL_BUILDER_H_

class MPICallBuilder {
public:
	MPICallBuilder();
	static SgExprStatement* buildMPISend(SgInitializedName* buffer,
			SgScopeStatement* scope,
			int partition,
			SgInitializedName* size = NULL);
	static SgExprStatement* buildMPIReceive(SgInitializedName* buffer,
			SgScopeStatement* scope,
			int partition,
			SgInitializedName* status,
			SgInitializedName* size = NULL);

private:
	static const char* getType(SgType* type);
};

#endif /* MPI_CALL_BUILDER_H_ */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
