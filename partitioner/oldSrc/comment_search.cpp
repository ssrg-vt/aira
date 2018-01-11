/*
 * comment_search.cpp - See comment_search.h.
 */

#include "rose.h" // Must come first for PCH to work.
#include "comment_search.h"

void CommentSearch::visit(SgNode *n)
{
  ROSE_ASSERT(n != NULL);

  // We may have already found a matching comment, if so do bother to look for
  // another.  This means that we return a match to the first instance of a
  // comment.  In the future this can be improved to find the Nth, but there is
  // no need right now.
  if (this->foundNode != NULL) return;

  SgLocatedNode *loc = isSgLocatedNode(n);
  if (loc == NULL) return; // Not all nodes support comments.

  AttachedPreprocessingInfoType *comments =
    loc->getAttachedPreprocessingInfo();
  if (comments == NULL) return; // If there are no comment we're done.

  AttachedPreprocessingInfoType::iterator i;
  for (i = comments->begin(); i != comments->end(); i++) {
    std::string cstr = (*i)->getString();
    if (cstr.find(*(this->searchString)) != std::string::npos) {
      // This comment contains a matching string, so record it.
      this->foundNode = n;
    }
  }
}

SgNode* CommentSearch::findComment(const std::string *string)
{
  ROSE_ASSERT(string != NULL);
  ROSE_ASSERT(foundNode == NULL);
  ROSE_ASSERT(searchString == NULL);
  this->searchString = new std::string(*string);
  this->traverseInputFiles(project, preorder);
  delete this->searchString; this->searchString = NULL;
  SgNode *foundNodeTmp = this->foundNode;
  this->foundNode = NULL;
  return foundNodeTmp;
}

SgNode* CommentSearch::findComment(const char *string)
{
  std::string str2(string);
  return findComment(&str2);
}
