// pass.cpp -- An LLVM pass to annotate calls and memory accesses.
//
// This file is distributed under the MIT license, see LICENSE.txt.
//
// This LLVM pass is fairly trivial, it walks over every function in the input
// program and annotates every call and every access with a call to the ptrack
// library (see ../library/).
//
// This enormously slows down the program, it could be sped up for loops that
// access memory serially by adding new hook to the ptrack library and
// exploiting LLVM loop information.  Generally however, the need to know the
// exact memory accessed at every single point makes optimisations hard.
//
// You are strongly recommended to optimise the input program (at least -O1,
// higher is okay as well) before running this pass.  Otherwise it will track a
// lot of memory values that are not actually interesting.

#define DEBUG_TYPE "ptrack"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::opt<bool> PTrackTrackFunctionEntry(
    "ptrack-function-entry", cl::init(false),
    cl::desc("Track every function entry"), cl::NotHidden);
static cl::opt<bool> PTrackTrackFunctionCalls(
    "ptrack-function-calls", cl::init(true),
    cl::desc("Track every function call"), cl::NotHidden);
static cl::opt<bool> PTrackTrackMemoryAccesses(
    "ptrack-memory-accesses", cl::init(true),
    cl::desc("Track every load and store"), cl::NotHidden);

STATISTIC(NumEnterTrackersInserted, "The # of function entry trackers inserted.");
STATISTIC(NumCallTrackersInserted, "The # of call trackers inserted.");
STATISTIC(NumMemTrackersInserted, "The # of memory trackers inserted.");

namespace {

class PartitionTracker : public FunctionPass {
public:
  static char ID;
  PartitionTracker() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F);

private:
  // Search a single basic flow for operations that need tracked.
  bool processBasicBlock(Function &, BasicBlock &);

  // Insert calls to the ptrack library for various conditions.
  bool trackMain(Function &);
  bool trackFunction(Function &);
  bool trackCall(Function &, Instruction &);
  bool trackMemoryAccess(Function &, Instruction &);
};

bool PartitionTracker::runOnFunction(Function &F) {
  bool insertedTracking = false;

  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    if (processBasicBlock(F, *BB)) insertedTracking = true;
  }

  // This must come *after* processBasicBlock, or the call inserted here will
  // also be tracked there.
  if (trackFunction(F)) insertedTracking = true;

  // This must come after trackFunction as it will insert init at the beginning
  // of the function (and if trackFunction is called after this function it
  // will insert a tracking call before the init function that will be inserted
  // here).
  if (F.getName() == "main") {
    if (trackMain(F)) insertedTracking = true;
  }

  return insertedTracking;
}

bool PartitionTracker::processBasicBlock(Function &F, BasicBlock &BB) {

  bool insertedTracking = false;

  for (BasicBlock::iterator i = BB.begin(), e = BB.end(); i != e; ++i) {
    if (i->mayReadOrWriteMemory()) {
      if (i->getOpcode() == Instruction::Call) {
        if (trackCall(F, *i)) insertedTracking = true;
      } else {
        if (trackMemoryAccess(F, *i)) insertedTracking = true;
      }
    }
  }

  return insertedTracking;
}

// Insert a call to initialise the tracking library.  No call is required to
// finalise the library as the initialiser will use atexit() for that purpose.
bool PartitionTracker::trackMain(Function &F) {
  // Skip over any allocas in the entry block, InsertPos will then point to the
  // first 'real' instruction so that the IRBuilder can insert instructions
  // just before that.
  BasicBlock *Entry = F.begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;
  IRBuilder<> Builder(InsertPos);

  // Build a reference to 'ptrack_init(void)'.
  FunctionType *FnType = FunctionType::get(Builder.getVoidTy(), false);
  Constant *Fn = F.getParent()->getOrInsertFunction("ptrack_init", FnType);

  // Finally, call the 'ptrack_init' function.
  Builder.CreateCall(Fn);

  return true;
}

// Insert a call to the tracking library at the entry point of this function.
bool PartitionTracker::trackFunction(Function &F) {
  if (!PTrackTrackFunctionEntry) return false;

  // Skip over any allocas in the entry block, InsertPos will then point to the
  // first 'real' instruction so that the IRBuilder can insert instructions
  // just before that.
  BasicBlock *Entry = F.begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;
  IRBuilder<> Builder(InsertPos);

  // Place the name of the function being processed into a global variable and
  // keep a symbolic pointer to it.
  Value *CurrentName = Builder.CreateGlobalStringPtr(F.getName());

  // Build a reference to 'ptrack_enter_func(const char *)'.
  SmallVector<Type *, 1> FnArgType;
  FnArgType.push_back(CurrentName->getType());
  FunctionType *FnType = FunctionType::get(Builder.getVoidTy(),
                                           FnArgType, false);
  Constant *Fn = F.getParent()->getOrInsertFunction("ptrack_enter_func",
                                                    FnType);

  // Finally, call the 'ptrack_enter_func' function.
  Builder.CreateCall(Fn, CurrentName);
  ++NumEnterTrackersInserted;
  return true;
}

// Insert a call to the tracking library for a single call-site.
bool PartitionTracker::trackCall(Function &F, Instruction &I) {
  if (!PTrackTrackFunctionCalls) return false;

  assert((I.getOpcode() == Instruction::Call) && "I is not a call instruction");
  CallInst *call = dyn_cast<CallInst>(&I);
  assert((call != nullptr) && "I is not a call instruction (?)");

  IRBuilder<> Builder(call);

  // Place the name of the function being processed (caller), and the name of
  // the function being called (callee) into a global variables and keep
  // symbolic pointers to them.
  Value *CallerName = Builder.CreateGlobalStringPtr(F.getName());
  Function *Callee = call->getCalledFunction();
  Value *CalleeName;
  if (Callee) {
    CalleeName = Builder.CreateGlobalStringPtr(Callee->getName());
  } else {
    CalleeName = Builder.CreateGlobalStringPtr("__ptrack_unknown_function");
  }


  // Build a reference to 'ptrack_call_func(const char *, const char *)'.
  SmallVector<Type *, 2> FnArgType;
  FnArgType.push_back(CallerName->getType());
  FnArgType.push_back(CalleeName->getType());
  FunctionType *FnType = FunctionType::get(Builder.getVoidTy(),
                                           FnArgType, false);
  Constant *Fn = F.getParent()->getOrInsertFunction("ptrack_call_func",
                                                    FnType);

  // Finally, call the 'ptrack_call_func' function.
  Builder.CreateCall2(Fn, CallerName, CalleeName);
  ++NumCallTrackersInserted;
  return true;
}

// Insert a call to the tracking library for a single memory access.
bool PartitionTracker::trackMemoryAccess(Function &F, Instruction &I) {
  if (!PTrackTrackMemoryAccesses) return false;

  // There are actually a wide variety of memory access instructions, we are
  // only really interested in generic loads and stores, most of the rest are
  // fairly specialised.  The obvious omission is the alloca instruction: it is
  // very common, but it is not relevant to tracking page accesses.
  std::string FnName;
  Value *Address;
  switch (I.getOpcode()) {
    case Instruction::Load:
      FnName = "ptrack_memory_read";
      Address = I.getOperand(0);
      break;
    case Instruction::Store:
      FnName = "ptrack_memory_write";
      Address = I.getOperand(1);
      break;
    default:
      // Ignore the rest for now ...
      return false;
  }

  // Insert the tracking call just before the memory access.
  IRBuilder<> Builder(&I);

  // TODO: We are clearly building up a large collection of identical global
  // strings, pooling these together is clearly desirable.
  Value *CurrentName = Builder.CreateGlobalStringPtr(F.getName());

  // Build a reference to 'ptrack_memory_[read,write](const char *,
  //                                                  const void *)'.
  SmallVector<Type *, 2> FnArgType;
  PointerType *VoidPtrTy = PointerType::getUnqual(Builder.getInt8Ty());
  FnArgType.push_back(CurrentName->getType());
  FnArgType.push_back(VoidPtrTy);
  FunctionType *FnType = FunctionType::get(Builder.getVoidTy(),
                                           FnArgType, false);
  Constant *Fn = F.getParent()->getOrInsertFunction(FnName,
                                                    FnType);

  // Cast the address to a void pointer and then call the function.
  Value *Cast = Builder.CreateCast(Instruction::BitCast, Address, VoidPtrTy);
  Builder.CreateCall2(Fn, CurrentName, Cast);

  ++NumMemTrackersInserted;
  return true;
}

}

char PartitionTracker::ID = 0;
static RegisterPass<PartitionTracker> X("ptrack", "Partition Tracking Pass",
                                        false, false);
