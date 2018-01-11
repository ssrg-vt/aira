/*
 * This pass performs any code cleanup necessary to prepare the source for
 * partitioning, including:
 *
 * 	-removing typedefs from variable declarations
 */

#include "rose.h"

#include "project_traversal.h"

int main(int argc, char** argv)
{
	//Setup the project
	SgProject* project = new SgProject(argc, argv);
	ROSE_ASSERT(project);
	AstTests::runAllTests(project);

	//Replace all typedefs with their canonical types
	ProjectTraversal pt;
	pt.traverse(project, preorder);

	return backend(project);
}
