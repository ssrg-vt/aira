/*
 * create_partition_mpi.cpp - TODO
 */

#include "rose.h" // Must come first for PCH to work.
#include <Cxx_Grammar.h>
#include "create_partition.h"
#include "comment_search.h"
#include <boost/lexical_cast.hpp>
#include "partitioning_constants.h"
#include <iostream>

using namespace SageBuilder;
using namespace SageInterface;

// TODO: This will break if calling 'partition' from an arbitrary location.
const char *empty_template = "templates/empty_partition_mpi.c";
const char *corrected_template = "templates/corrected_empty_partition_mpi.c";

/*
 * Partition constructor - Create an empty partition.
 *
 * Note: Yes, we really do set an unsigned variable (currentFuncNum) to -1.
 * This is well-defined, what actually happens is that it gets set to UINT_MAX,
 * we increment it before using it, overflow is defined for unsigned variables,
 * so the first use is 0.  (It is well-defined in C, I've not checked the C++
 * standard).
 */
MPIPartition::MPIPartition(const std::string &name)
  : Partition(name, empty_template), currentFuncNum(-1)
{
  DEBUG("Creating MPI Partition");
}

MPIPartition::~MPIPartition()
{
  DEBUG("Destroying MPI Partition");
}

/*
 * moveFunction - TODO
 */
Partition::FCode MPIPartition::moveFunction(SgFunctionDeclaration *F)
{
  ROSE_ASSERT(F != NULL);
  DEBUG("Moving function " << F->get_name().getString() << " into partition.");

  this->currentFuncNum += 1; // Next function!
  // TODO: Do we actually want to create two copies of the function?  One for
  //       an MPI remote launch, and one for internal use within the partition
  //       (recursion, a different function using it).
  SgFunctionDeclaration *newF = createEmptyFunction();
  insertFunctionBody(F, newF);
  insertCallToFunction(newF);

  return 1; // TODO: Return the correct code.
}

/*
 * addInput - TODO
 */
void MPIPartition::addInput(Partition::FCode F, SgType *type, size_t size)
{
  // TODO ROSE_ASSERT(F is valid);
  ROSE_ASSERT(type != NULL);
  ROSE_ASSERT(size > 0);

  // TODO
}

/*
 * addOutput - TODO
 */
void MPIPartition::addOutput(Partition::FCode F, SgType *type, size_t size)
{
  // TODO ROSE_ASSERT(F is valid);
  ROSE_ASSERT(type != NULL);
  ROSE_ASSERT(size > 0);

  // TODO
}

/*
 * Return the partition number
 */
int MPIPartition::getPartitionNumber()
{
	return MPI;
}

/*
 * getCurrentFuncName - Does what it says on the tin.
 *
 * TODO: Return a pointer?
 */
std::string MPIPartition::getCurrentFuncName()
{
  unsigned fNum = this->currentFuncNum;
  std::string name =   *(this->name) + "_f_"
                     + boost::lexical_cast<std::string>(fNum);
  return name;
}

/*
 * createEmptyFunction - Insert an empty shell function into the template.
 */
SgFunctionDeclaration* MPIPartition::createEmptyFunction()
{
  SgName name = getCurrentFuncName();
  DEBUG("    ... creating an empty function stub '" << name.getString()
        << "'.");
  //SgFile *file = (*(this->project))[0];
  //SgGlobal *scope = getGlobalScope(file); // TODO: This doesn't work, why???
  SgGlobal *scope = getFirstGlobalScope(project); // TODO can the partition assume first == okay?
  ROSE_ASSERT(scope->supportsDefiningFunctionDeclaration()); // TODO ASSERTS !!!
  pushScopeStack(scope);
  SgFunctionDeclaration *newF = buildDefiningFunctionDeclaration(
                                                  name, buildVoidType(),
                                                  buildFunctionParameterList(),
                                                  scope);
  SgNode *insertAfter = this->commentSearch->findComment("@@NEW_FUNCTIONS_HERE@@");
  // TODO: Insert this at the correct place.
  // ROSE_ASSERT(insertAfter != NULL);
  appendStatement(newF);
  popScopeStack();
  return newF;
}

/*
 * insertFunctionBody - Move the body of the function to be partitioned into
 * the shell function within the partition.
 */
void MPIPartition::insertFunctionBody(SgFunctionDeclaration *oldF,
                                      SgFunctionDeclaration *newF)
{
  DEBUG("    ... moving function body into partition.")

  // TODO: Should this copy rather than link?

  // Take the body of the old function and use that for the new function.
  SgFunctionDefinition *def = oldF->get_definition();
  ROSE_ASSERT(def != NULL);
  newF->set_definition(def);

  // Link the moved definition against the new function.
  def->set_parent(newF);
}

/*
 * insertCallToFunction - add a given function to the 'while (!finished) { ...
 * }' MPI loop.
 */
void MPIPartition::insertCallToFunction(SgFunctionDeclaration *F)
{
  SgNode *insertAfter = this->commentSearch->findComment("@@NEW_CASES_HERE@@");
  // TODO: Insert this at the correct place.
  // ROSE_ASSERT(insertAfter != NULL);
  // TODO
}

/*
 * setMPIHeader - set the MPI header location for the MPI template (allows the
 * user to specify a custom location or defaults to mpi.h)
 */
void MPIPartition::setMPIHeader(const std::string &mpi_header)
{
	std::cerr << "Partition: Setting MPI header location in template file"
			<< std::endl;

	std::fstream template_file;
	std::stringstream template_content;
	std::string template_string;
	const char* header_comment = "// @@MPI_INCLUDE_HERE@@";
	const int comment_length = 23;
	std::string header_string = "#include <" + mpi_header + ">";

	template_file.open(empty_template, std::ios::in);
	if(!template_file.is_open())
	{
		std::cerr << "Partition: Could not set MPI header location!"
				<< std::endl;
		return;
	}
	template_content << template_file.rdbuf();
	template_string = template_content.str();
	template_file.close();

	template_file.open(corrected_template, std::ios::out);
	if(!template_file.is_open())
	{
		std::cerr << "Partition: Could not set MPI header location!"
				<< std::endl;
		return;
	}
	template_file << template_string.replace(
			template_string.find(header_comment), comment_length, header_string);

	template_file.close();

	empty_template = corrected_template;
}

/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
