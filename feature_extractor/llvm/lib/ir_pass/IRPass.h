/*****************************************************************************/
/* IRPass - iterate through each function in the IR and extract features     */
/*****************************************************************************/

#ifndef _IR_PASS_H
#define _IR_PASS_H

class IRPass : public llvm::ModulePass
{
public:
	IRPass() : llvm::ModulePass(ID) {};

	/* Callbacks to do work */
	virtual bool doInitialization(llvm::Module &M);
	virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
	virtual bool runOnModule(llvm::Module& M);
	virtual bool doFinalization(llvm::Module &M);

	static char ID;

private:
};

#endif /* _IR_PASS_H */
