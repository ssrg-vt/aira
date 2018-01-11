/*
 * create_partition.cpp - TODO
 */

#include "rose.h" // Must come first for PCH to work.
#include <Cxx_Grammar.h>
#include "create_partition.h"

Partition::Partition(const std::string &name, const std::string &empty_template)
  : project(NULL),
    name(NULL),
    commentSearch(NULL)
{
  this->name = new std::string(name);
  createEmptyPartition(empty_template);
}

/*
 * Partition destructor.
 */
Partition::~Partition()
{
  DEBUG("Destroying partition.");
  delete this->project;
  delete this->name;
  delete this->commentSearch;
}

/*
 * createEmptyPartition - Helper function to help create an empty partition.
 *
 * It does this by loading the AST for a template 
 */
void Partition::createEmptyPartition(const std::string &empty_template)
{
  DEBUG("Creating partition.");
  std::vector<std::string> args(2);
  args[0] = std::string("partition");
  args[1] = empty_template;
  project = frontend(args);
  ROSE_ASSERT(project != NULL);
  // TODO: Make this optional?:
  AstTests::runAllTests(project);
  // TODO: This has no effect???:
  project->set_outputFileName(std::string("Partition_") + *name + ".c");
  commentSearch = new CommentSearch(project);
}

/*
 * finalize - TODO
 */
void Partition::finalize()
{
  //AstTests::runAllTests(project); // Rerun tests. TODO: This fails???
  // TODO: This should really be optional:
  // TODO: How to set DOT graph name according to output name?:
  generateDOT(*project, std::string("_Partition_") + *name);
  DEBUG("Generating C for this partition.");
  project->unparse();
}
/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
