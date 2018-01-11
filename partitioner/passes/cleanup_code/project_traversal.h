/*
 * project_traversal.h - defines the interface for traversing a project to
 * perform any necessary code cleanup.
 *
 *  Created on: Jul 16, 2013
 *      Author: Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef PROJECT_TRAVERSAL_H_
#define PROJECT_TRAVERSAL_H_

class ProjectTraversal : public AstSimpleProcessing
{
public:
	ProjectTraversal();

	virtual void visit(SgNode* node);

private:
	void removeTypedef(SgVariableDeclaration* varDecl);
	void cleanupExpression(SgExpression* expr);
};

#endif /* PROJECT_TRAVERSAL_H_ */
