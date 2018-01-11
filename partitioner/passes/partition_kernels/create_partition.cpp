/*
 * create_partition.cpp - TODO
 */

#include "rose.h" // Must come first for PCH to work.

#include "common.h"
#include "partition_kernels_common.h"
#include "create_partition.h"

/*
 * Default generic partition construction - build the project from the
 * specified template file.
 */
Partition::Partition(const string& p_name, const string& emptyTemplate, const string& templateOutput) :
	project(NULL),
	templateFile(NULL),
    name(NULL)
{
  name = new string(p_name);
  createEmptyPartition(emptyTemplate, templateOutput);

  //Find the template file
  SgFilePtrList& files(project->get_fileList());
  SgFilePtrList::const_iterator fileIt;
  SgFile* file;
  for(fileIt = files.begin(); fileIt != files.end(); fileIt++)
  {
	file = isSgFile(*fileIt);
	ROSE_ASSERT(file);

	if(stripPathFromFileName(file->getFileName()) == stripPathFromFileName(emptyTemplate))
	{
      templateFile = isSgSourceFile(file);
      break;
	}
  }
  ROSE_ASSERT(templateFile);
}

/*
 * Partition destructor.
 */
Partition::~Partition()
{
  delete project;
  delete name;
}

/*
 * Returns a string with the name of the function specified by the function
 * code, or an empty string otherwise.
 */
string Partition::getFuncName(FCode funcNum)
{
	map<FCode, SgFunctionDeclaration*>::const_iterator it;
	it = funcs.find(funcNum);
	if(it != funcs.end())
		return NAME(it->second);
	else
		return "";
}

/*
 * createEmptyPartition - Helper function to help create an empty partition.
 *
 * It does this by loading the AST from a template file
 */
void Partition::createEmptyPartition(const string &emptyTemplate, const string& outputFile)
{
  string msg = "Building partition AST for " + *name;
  DEBUG(TOOL, msg);

  vector<string> args;
  args.push_back(*name);
  args.push_back(emptyTemplate);
  if(outputFile != "")
  {
	  msg = "Setting output file: " + outputFile;
	  DEBUG(TOOL, msg);

	  //Set output file, if specified
	  args.push_back("-rose:o");
	  args.push_back(outputFile);
  }

  project = frontend(args);
  ROSE_ASSERT(project != NULL);
  AstTests::runAllTests(project); // TODO: Make this optional?
}

/*
 * finalize - write a DOT file representing the AST and generate the partition
 * code
 */
void Partition::finalize()
{
  string msg = "Checking transformed AST for " + *name;
  DEBUG(TOOL, msg);

  //TODO this fails for GPU partitioning, complaining that initialized names
  //inserted into parameter lists don't have a scope (scope == null)
  //AstTests::runAllTests(project);

  // TODO: This should really be optional
  // TODO: How to set DOT graph name according to output name?:
  if(false)
  {
	  msg = "Generating DOT file for " + *name;
	  DEBUG(TOOL, msg);
	  generateDOT(*project, string("_Partition_") + *name);
  }

  msg = "Generating code for " + *name;
  DEBUG(TOOL, msg);
  project->unparse();
}

/*
 * Internal method
 *
 * Sets the specified header comment to the specified header in the partition
 * source.
 */
void Partition::setHeader(const string& header, const string& headerComment,
		const string& templateName, const string& correctedTemplateName)
{
	string msg = "Setting header location in template file to \"" + header + "\"";
	DEBUG(TOOL, msg);

	fstream templateFile;
	stringstream templateContent;
	string templateString;
	string headerString = "#include <" + header + ">";

	templateFile.open(templateName.c_str(), ios::in);
	if(!templateFile.is_open())
	{
		WARNING(TOOL, "Could not open template file to set header location");
		return;
	}
	templateContent << templateFile.rdbuf();
	templateString = templateContent.str();
	templateFile.close();

	templateFile.open(correctedTemplateName.c_str(), ios::out);
	if(!templateFile.is_open())
	{
		WARNING(TOOL, "Could not write corrected template file");
		return;
	}
	templateFile << templateString.replace(templateString.find(headerComment),
			headerComment.length(), headerString);
	templateFile.close();
}
/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
