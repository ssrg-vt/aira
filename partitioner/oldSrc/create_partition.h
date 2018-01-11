/*
 * create_partition.h - TODO
 */

#ifndef _CREATE_PARTITION_H
#define _CREATE_PARTITION_H

#include "comment_search.h"

/*
 * DEBUG - A helper macro to help track what is going on.
 */
#define DEBUG(MSG) std::cerr << "Partition " << *(this->name) << ": " << MSG << std::endl;

class Partition {
  public:
    typedef int FCode; // In case this needs to be a struct in the future.

  protected:
    SgProject *project;
    std::string *name;
    CommentSearch *commentSearch;

    // Yes, the constructor is not public!  Use a sub-class.
    Partition(const std::string &name, const std::string &empty_template);
  public:
    virtual ~Partition();

    virtual FCode moveFunction(SgFunctionDeclaration *F) = 0;
    virtual void addInput(FCode F, SgType *type, size_t size = 1) = 0;
    virtual void addOutput(FCode F, SgType *type, size_t size = 1) = 0;
    virtual int getPartitionNumber() = 0;
    void finalize();

  private:
    void createEmptyPartition(const std::string &empty_template);
};

class MPIPartition : public Partition {
  private:
    unsigned currentFuncNum;

  public:
    MPIPartition(const std::string &name);
    virtual ~MPIPartition();

    virtual FCode moveFunction(SgFunctionDeclaration *F);
    virtual void addInput(FCode F, SgType *type, size_t size = 1);
    virtual void addOutput(FCode F, SgType *type, size_t size = 1);
    virtual int getPartitionNumber();

    static void setMPIHeader(const std::string &mpi_header);

  private:
    std::string getCurrentFuncName();
    SgFunctionDeclaration* createEmptyFunction();
    void insertFunctionBody(SgFunctionDeclaration *OldF,
                            SgFunctionDeclaration *NewF);
    void insertCallToFunction(SgFunctionDeclaration *F);
};

#endif // _CREATE_PARTITION_H
/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
