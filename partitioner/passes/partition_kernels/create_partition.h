/*
 * create_partition.h - declaration for the classes used to build partitions
 * for various architectures.
 */

#ifndef _CREATE_PARTITION_H
#define _CREATE_PARTITION_H

class Partition {
  public:
    typedef int FCode; // In case this needs to be a struct in the future.

  protected:
    SgProject* project;
    SgSourceFile* templateFile;
    string* name;

    map<FCode, SgFunctionDeclaration*> funcs;
    FCode currentFuncNum;
    set<SgClassType*> classes;

    // Yes, the constructor is not public!  Use a sub-class
    Partition(const string& p_name, const string& emptyTemplate, const string& templateOutput = "");

    static void setHeader(const string& header, const string& comment,
    		const string& templateName, const string& correctedTemplateName);

    virtual SgFunctionDeclaration* createEmptyFunction(SgFunctionDeclaration* func, bool) = 0;
    virtual void copyFunctionBody(SgFunctionDeclaration*, SgFunctionDeclaration*) = 0;

  public:
    string getFuncName(FCode funcNum);

    virtual ~Partition();

    virtual FCode moveFunction(SgFunctionDeclaration*) = 0;
    virtual FCode moveSupportingFunction(SgFunctionDeclaration*) = 0;
    virtual void addClassDeclaration(SgClassType*) = 0;
    virtual enum Hardware getPartitionType() = 0;
    virtual void finalizeFunction(FCode funcNum) = 0;
    void finalize();

  private:
    void createEmptyPartition(const string& emptyTemplate, const string& outputFile = NULL);
};

class MPIPartition : public Partition {
  public:
    MPIPartition(const string&, const string&);
    virtual ~MPIPartition();

    /* Implemented virtual functions */
    virtual FCode moveFunction(SgFunctionDeclaration*);
    virtual FCode moveSupportingFunction(SgFunctionDeclaration*);
    virtual enum Hardware getPartitionType();
    virtual void addClassDeclaration(SgClassType*);
    virtual void finalizeFunction(FCode funcNum);

    /* Functions for adding inputs/outputs */
    void addInput(FCode, SgInitializedName*);
    void addOutput(FCode, SgInitializedName*);
    void addGlobalInput(FCode, SgInitializedName*);
    void addGlobalOutput(FCode, SgInitializedName*);

    /* Static class functions */
    static void setMPIHeader(const string&);

  protected:
    virtual SgFunctionDeclaration* createEmptyFunction(SgFunctionDeclaration* func, bool);
    virtual void copyFunctionBody(SgFunctionDeclaration*, SgFunctionDeclaration*);

  private:
    SgInitializedName* statusVar;
    SgVariableDeclaration* statusVarDecl;
    map<string, SgVariableDeclaration*> globalVars;
    map<string, SgInitializedName*> mallocedVars;
    map<string, SgClassType*> classes;
    vector<SgStatement*> inputStmts;
    vector<SgStatement*> outputStmts;
    SgSwitchStatement* functionSelect;

    map<string, SgVariableDeclaration*> mpiDatatypes;
    SgFunctionDeclaration* datatypeDefFunc;
    SgFunctionDefinition* datatypeDefBody;

    static bool correctedHeader;

    void insertCallToFunction(SgFunctionDeclaration*, FCode funcNum);

    void addClassMPITransferCalls(vector<SgStatement*>& stmts, SgFunctionDefinition* funcDef,
    		bool isInput, SgInitializedName* var, SgClassType* type, SgInitializedName* sizeVar = NULL);
    void addClassMPITransferCalls(vector<SgStatement*>& stmts, SgFunctionDefinition* funcDef,
        	bool isInput, SgExpression* var, SgClassType* type, SgExpression* sizeVar = NULL);
    void freeVar(SgInitializedName* buffer, SgFunctionDeclaration* func);
    void addInputInternal(SgFunctionDefinition* funcDef, SgInitializedName* buffer);
    void addOutputInternal(SgFunctionDefinition* funcDef, SgInitializedName* buffer);
    SgCastExp* buildMallocCall(SgName, SgType*, SgScopeStatement*);
    SgCastExp* buildMallocCall(SgExpression*, SgType*, SgScopeStatement*);
    SgExprStatement* buildFreeCall(SgInitializedName*, SgScopeStatement*);
    SgExprStatement* buildFreeCall(SgExpression*, SgScopeStatement*);
};

class GPUPartition : public Partition {
  public:
	GPUPartition(const string &, const string&);
	virtual ~GPUPartition();

	virtual FCode moveFunction(SgFunctionDeclaration*);
	virtual FCode moveSupportingFunction(SgFunctionDeclaration*);
	virtual void addClassDeclaration(SgClassType*);
	virtual enum Hardware getPartitionType();
	virtual void finalizeFunction(FCode funcNum);

	void addGlobalVar(SgInitializedName* var);

	static void setGPUHeader(const string& gpuHeader);
	static void setGPUErrorHeader(const string& gpuErrHeader);

  protected:
	virtual SgFunctionDeclaration* createEmptyFunction(SgFunctionDeclaration* func, bool);
	void copyFunctionBody(SgFunctionDeclaration*, SgFunctionDeclaration*);

  private:
	SgDeclarationStatement* errCode;
	set<SgInitializedName*> globalVars;

	static bool correctedHeader;

	void setParameterList(SgFunctionDeclaration*, SgFunctionDeclaration*);
};

#endif // _CREATE_PARTITION_H
/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
