/*
 * comment_search.h - TODO
 */

#ifndef _COMMENT_SEARCH_H
#define _COMMENT_SEARCH_H

class CommentSearch : public AstSimpleProcessing {
  private:
    SgProject *project;
    SgNode *foundNode;
    std::string *searchString;

  public:
    CommentSearch(SgProject *_project) : project(_project), foundNode(NULL),
                                         searchString(NULL) {}

    SgNode *findComment(const std::string *string);
    SgNode *findComment(const char *string);

  protected:
    virtual void visit(SgNode *n);
};

#endif // _COMMENT_SEARCH_H
