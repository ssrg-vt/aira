/*
 * main.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: rlyerly
 */

#include "rose.h"

#include "common.h"
#include "partition_kernels_common.h"
#include "program_options.h"
#include "preprocessor_handler.h"
#include "pragma_handling.h"
#include "arch_selector.h"
#include "partition_kernel.h"
#include "mpi_call_builder.h"

int main(int argc, char** argv)
{
	//Parse command-line args
	ProgramOptions args(argc, argv);

	//Initialize the project
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project);
	AstTests::runAllTests(project);
	bool isMPIProgram = false;

	//Insert preprocessor information
	PreprocessorHandler ph(args.getMpiHeaderLocation());
	SgFile* mainFile = getEnclosingFileNode(findMain(project));
	ROSE_ASSERT(mainFile != NULL);
	ph.insertMPIHeader(mainFile);

	//Initialize/create partitions
	if(args.getMpiHeaderLocation() != "")
		MPIPartition::setMPIHeader(args.getMpiHeaderLocation());
	if(args.getGpuHeaderLocation() != "")
		GPUPartition::setGPUHeader(args.getGpuHeaderLocation());
	if(args.getGpuErrorHeaderLocation() != "")
		GPUPartition::setGPUErrorHeader(args.getGpuErrorHeaderLocation());

	string mpiName = "tilera";
	if(args.getMpiPartitionName() != "")
		mpiName = args.getMpiPartitionName();
	string gpuName = "cuda";
	if(args.getGpuPartitionName() != "")
		gpuName = args.getGpuPartitionName();
	MPIPartition* tilera = new MPIPartition(mpiName, args.getMpiOutputFile());
	GPUPartition* cuda = new GPUPartition(gpuName, args.getGpuOutputFile());

	//Partition kernels
	Rose_STL_Container<SgNode*> pragmas = querySubTree(project, V_SgPragmaDeclaration);
	Rose_STL_Container<SgNode*>::const_iterator pragmaIt;
	SgPragmaDeclaration* pragma;
	SgFunctionDeclaration* funcDecl;
	SgStatement* stmt;
	for(pragmaIt = pragmas.begin(); pragmaIt != pragmas.end(); pragmaIt++)
	{
		pragma = isSgPragmaDeclaration(*pragmaIt);
		ROSE_ASSERT(pragma);

		PragmaParser pp(pragma);
		if(pp.isPopcornPragma() && pp.getPragmaType() == ARCH)
		{
			//Get pragmas for function declaration
			stmt = getNextStatement(pragma);
			while(!isSgFunctionDeclaration(stmt) && isSgPragmaDeclaration(stmt))
				stmt = getNextStatement(stmt);
			funcDecl = isSgFunctionDeclaration(stmt);
			if(!funcDecl)
			{
				string msg = "Statement following popcorn pragmas is not a function declaration!";
				DEBUG(TOOL, msg);
				continue;
			}
			Pragmas pragmas(funcDecl);

			//Get architectures selected
			ArchSelector archSelect(*pragmas.getArchs(), *pragmas.getCompatibleArchs());

			//Partition function
			PartitionKernel pk(funcDecl, pragmas);
			if(archSelect.partitionOntoGpu())
				pk.partitionToGPU(cuda);
			if(archSelect.partitionOntoMpi())
			{
				pk.partitionToMPI(tilera);
				isMPIProgram = true;

				//TODO for some reason this won't add the header to other files, despite
				//debug messages stating the opposite...
				ph.insertMPIHeader(getEnclosingFileNode(funcDecl));
			}
			pk.annotate();
		}
	}

	//Instrument with MPI initialize/finalize calls and includes
	if(isMPIProgram)
		MPICallBuilder::instrumentMainForMPI(findMain(project), MPI);

	//Generate partition code
	tilera->finalize();
	cuda->finalize();

	delete tilera;
	delete cuda;

	//Try unparsing the project?
	project->unparse();
	return 0;
	//return backend(project);
}
