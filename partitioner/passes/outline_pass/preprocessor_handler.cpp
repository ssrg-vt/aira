/*
 * preprocessor_handler.cpp
 *
 *  Created on: Jun 10, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "partition_kernels_common.h"
#include "preprocessor_handler.h"

/*
 * Default constructor.
 */
PreprocessorHandler::PreprocessorHandler(string p_mpiHeaderString) :
	mpiHeaderString(p_mpiHeaderString),
	mpiHeader(NULL),
	ompiFix(NULL),
	searchingForExistingHeader(false),
	foundExistingHeader(false)
{
	// no-op for now
	if(mpiHeaderString == "")
		mpiHeaderString = MPI_HEADER;
}

/*
 * Insert the MPI header (and an OpenMPI fix for EDG) into the specified
 * file.
 */
void PreprocessorHandler::insertMPIHeader(SgFile* file, bool addEDGFix)
{
	SgSourceFile* sourceFile = isSgSourceFile(file);
	ROSE_ASSERT(sourceFile != NULL);

	searchingForExistingHeader = true;
	traverse(sourceFile->get_globalScope(), preorder);
	searchingForExistingHeader = false;

	if(!foundExistingHeader)
	{
		string msg = "Adding MPI header location to file "
				+ stripPathFromFileName(file->getFileName());
		DEBUG(TOOL, msg);

		//Insert MPI header
		mpiHeader = insertHeader(mpiHeaderString, PreprocessingInfo::after, true,
				sourceFile->get_globalScope());

		//Insert define macro so that EDG doesn't complain about OpenMPI
		if(addEDGFix)
			traverse(sourceFile->get_globalScope(), preorder);
	}
	foundExistingHeader = false;
}

/*
 * Traverse the AST to find where MPI headers have been inserted, and insert
 * fixes when using OpenMPI + EDG.
 *
 * Note: Unfortunately we have to do an entire AST traversal at this point
 * because there does not appear to be a good way to find the headers
 * otherwise.
 */
void PreprocessorHandler::visit(SgNode* node)
{
	SgLocatedNode* locatedNode = isSgLocatedNode(node);
	bool found = false;
	if(locatedNode)
	{
		AttachedPreprocessingInfoType* ppi = locatedNode->getAttachedPreprocessingInfo();
		if(ppi)
		{
			AttachedPreprocessingInfoType::iterator it = ppi->begin();

			if(searchingForExistingHeader)
			{
				for(it = ppi->begin(); it != ppi->end(); it++)
				{
					if((*it)->getString().rfind(MPI_STR) != string::npos)
					{
						foundExistingHeader = true;
						//TODO abort traversal
						break;
					}
				}
			}
			else
			{
				//Search this preprocessing info for the MPI header
				for(it = ppi->begin(); it != ppi->end(); it++)
				{
					if((*it) == mpiHeader)
					{
						DEBUG(TOOL, "\tAdding define so that EDG doesn't complain");

						//Create OpenMPI define so EDG doesn't complain (automatically appended)
						ompiFix = buildCpreprocessorDefineDeclaration(locatedNode, OMPI_FIX,
								PreprocessingInfo::before);
						found = true;
						break;
					}
				}

				if(!found) //MPI header wasn't in this preprocessing info
					return;

				//Re-traverse the list to reorder MPI header and above define
				int headerLoc = -1, defineLoc = -1;
				unsigned int x = 0;
				PreprocessingInfo* info = NULL;
				for(x = 0; x < ppi->size(); x++)
				{
					info = (*ppi)[x];
					if(info == mpiHeader)
						headerLoc = x;
					else if(info == ompiFix)
						defineLoc = x;
				}
				ROSE_ASSERT(headerLoc >= 0);
				ROSE_ASSERT(defineLoc >= 0);

				info = (*ppi)[headerLoc];
				(*ppi)[headerLoc] = (*ppi)[defineLoc];
				(*ppi)[defineLoc] = info;

				//TODO abort traversal
			}
		}
	}
}
