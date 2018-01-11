/*
 * preprocessor_handler.h
 *
 *  Created on: Jun 10, 2013
 *      Author: rlyerly
 */

#ifndef PREPROCESSOR_HANDLER_H_
#define PREPROCESSOR_HANDLER_H_

#define MPI_HEADER "mpi.h"
#define OMPI_FIX "#define OMPI_DECLSPEC"

class PreprocessorHandler : public AstSimpleProcessing {
public:
	PreprocessorHandler(string p_mpiHeaderString = MPI_HEADER);
	void insertMPIHeader(SgFile* file, bool addEDGFix = true);
	void visit(SgNode* node);

private:
	string mpiHeaderString;
	PreprocessingInfo* mpiHeader;
	PreprocessingInfo* ompiFix;
	bool searchingForExistingHeader;
	bool foundExistingHeader;
};

#endif /* PREPROCESSOR_HANDLER_H_ */
