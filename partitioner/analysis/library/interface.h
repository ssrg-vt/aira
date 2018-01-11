// interface.h -- The functions inserted by the LLVM transformation.
//
// This file is distributed under the MIT license, see LICENSE.txt.
//
// These are the core tracking functions that are used to track which functions
// call which, and which memory addresses each function accesses.

#ifndef _INTERFACE_H
#define _INTERFACE_H

extern "C" {

// Called once, before any other function.  It will initialise data structures
// and register a callback to the destruction function to be called upon
// completion.
void ptrack_init(void);

// To be (optionally) called upon entering a function.  NOTE: In the current
// implementation this has no effect.
void ptrack_enter_func(const char *fname);

// To be called just before 'caller' calls 'callee'.
void ptrack_call_func(const char *caller, const char *callee);

// To be called just before 'fname' reads from 'addr'.
void ptrack_memory_read(const char *fname, const void *addr);

// To be called just before 'fname' writes to 'addr'.
void ptrack_memory_write(const char *fname, const void *addr);

} // End of extern "C"

#endif // _INTERFACE_H
