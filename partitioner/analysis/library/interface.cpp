// interface.cpp -- See interface.h
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "interface.h"
#include "page_tracking.h"
#include "function_graph.h"
#include "cost.h"

//#define DEBUG(x) { x; }
#define DEBUG(x)

static void ptrack_memory(const char *fname, const void *addr);
void ptrack_destroy(void);

static PageTracker *pt = nullptr;
static FunctionGraph *fg = nullptr;

// Compute node names should always start with a '&' character.
static node_t xeon = "&& Xeon &&";
static node_t phi  = "&& Xeon-Phi &&";

extern "C" {

void ptrack_init(void)
{
  DEBUG(std::cerr << "*** Initialising PTrack" << std::endl);
  atexit(&ptrack_destroy);

  // Initialise the data structures, costs come from cost.h.
  pt = new PageTracker();
  fg = new FunctionGraph(migrationCost, pageFaultCost,
                         xeon, xeonCost,
                         phi, phiCost);
}

void ptrack_enter_func(const char *fname)
{
  DEBUG(std::cerr << "*** Entering function " << fname << std::endl);
  assert(fname && "Invalid use of interface (null function name)");

  // Do nothing.
}

void ptrack_call_func(const char *caller, const char *callee)
{
  DEBUG(std::cerr << "*** Function '" << caller << "' calling function '"
                  << callee << "'" << std::endl);
  assert(caller && "Invalid use of interface (null function name)");
  assert(callee && "Invalid use of interface (null function name)");
  assert((fg != nullptr) && "ptrack has not been initialised");

  // LLVM inserts functions for certain tasks (e.g. memset for initialising
  // memory).  These have nothing to do with partitioning (they can happen
  // locally for any architecture), and we do not see the implementation of
  // them, so quietly ignore them.  Failing to do this slightly biases against
  // partitioning, as these nodes tend to be widely used, and thus add a false
  // cost to putting functions on a secondary architecture.
  if (strncmp(callee, "llvm.", 5) == 0) return;

  fg->call(caller, callee);

  // Our tracking tool can not track the memory/computational cost of library
  // functions.  Mostly this doesn't matter, as things like printf are not
  // interesting for partitioning.  Libm, however, is!  Computational kernels
  // frequently use math library functions, and assuming that they have a cost
  // of 0 is clearly wrong so we give them a cost.
  // TODO: Have a better way of accounting for the costs of library functions.
  const edge_t libm = 3;
  if (NODE_T_EQ(callee, "sqrt")) fg->incurPageFault(callee, callee, libm);
  else if (NODE_T_EQ(callee, "pow")) fg->incurPageFault(callee, callee, libm);
  else if (NODE_T_EQ(callee, "log")) fg->incurPageFault(callee, callee, libm);
  else if (NODE_T_EQ(callee, "exp")) fg->incurPageFault(callee, callee, libm);
  else if (NODE_T_EQ(callee, "sin")) fg->incurPageFault(callee, callee, libm);
  else if (NODE_T_EQ(callee, "cos")) fg->incurPageFault(callee, callee, libm);
}

void ptrack_memory_read(const char *fname, const void *addr)
{
  DEBUG(std::cerr << "*** Function '" << fname << "' read memory address "
                  << addr << " (page " << PageTracker::memToPage(addr) << ")"
                  << std::endl);
  assert(fname && "Invalid use of interface (null function name)");
  ptrack_memory(fname, addr);
}

void ptrack_memory_write(const char *fname, const void *addr)
{
  DEBUG(std::cerr << "*** Function '" << fname << "' wrote to memory address "
                  << addr << " (page " << PageTracker::memToPage(addr) << ")"
                  << std::endl);
  assert(fname && "Invalid use of interface (null function name)");
  ptrack_memory(fname, addr);
}

} // End of extern "C"

// We don't treat reads and writes differently currently, so just have one copy
// of the code.
static void ptrack_memory(const char *fname, const void *addr)
{
  assert((pt != nullptr) && "ptrack has not been initialised");
  assert((fg != nullptr) && "ptrack has not been initialised");

  node_t functionHoldingPage = pt->functionAccessesMemory(fname, addr);

  // Note the we always record a page-fault, even if this function already
  // holds the page (i.e. no page fault actually occurs).  FunctionGraph knows
  // the difference.  We only print true potential page-faults to the debug log
  // though.
  DEBUG({
    if (!NODE_T_EQ(fname, functionHoldingPage)) {
      std::cerr << "***     - Potential page fault" << std::endl;
    }
  });

  fg->incurPageFault(fname, functionHoldingPage);
}

// Load the file specified by the PTRACK_PARALLEL environment variable and set
// every function contained within as parallel.
static void ptrack_load_parallel(void)
{
  const char *parallel_file = getenv("PTRACK_PARALLEL");
  if (parallel_file == NULL) {
    std::cerr << "### WARNING: PTRACK_PARALLEL was not specified." << std::endl;
  } else {
    std::ifstream f(parallel_file);
    if (f.fail()) {
      std::cerr << "### WARNING: PTRACK_PARALLEL file could not be read."
                << std::endl;
    } else {
      std::cout << "### Loading parallelism data from '" << parallel_file
                << "'." << std::endl;
      fg->loadParallelFunctions(f);
    }
  }
}

// Library functions have no compute cost, and thus can easily get dragged
// along with a partitioning even if that is a silly idea (libc functions like
// printf or fopen should only be run on the Xeon Phi if there is a good
// reason).  So for now we force them all onto the Xeon.
// TODO: Handle this intelligently.
static void ptrack_pin(UndirectedGraph *g)
{
#define PIN(F) if (g->exists((F))) g->addEdge(xeon, (F), ((edge_t)1) << 60);
  PIN("fopen");
  PIN("fclose");
  PIN("fputc");
  PIN("fputs");
  PIN("putc");
  PIN("putchar");
  PIN("puts");
  PIN("printf");
  PIN("fprintf");
  PIN("fread");
  PIN("fwrite");
  PIN("fseek");
  PIN("unlink");
  PIN("gettimeofday");
  PIN("settimeofday");

  // TODO: Is it really accurate to pin the following functions to the Xeon?
  // They should run on the Xeon-Phi just fine.
  PIN("sprintf");
  PIN("atoi");
  PIN("malloc");
  PIN("calloc");
  PIN("free");
  PIN("strcmp");
  PIN("strncmp");
  PIN("strcat");
  PIN("strncat");
  PIN("strcpy");
  PIN("strncpy");
  PIN("strchr");

#undef PIN
}

// Build the cost graph, perform the partitioning, print out useful information
// (most importantly: what function to run on the Xeon Phi) and finally output
// the DOT graph.
static void ptrack_partition(void)
{
  UndirectedGraph *g = fg->buildCostGraph();

  // Pin some libc functions to the Xeon.
  ptrack_pin(g);

  // Fin the min-cut of the graph such that the fake cost nodes are on opposite
  // sides of the cut.
  edge_t cost = g->partition(xeon, phi);
  std::cout << "### Partitioning has cost " << cost << "." << std::endl;

  std::cout << "### Functions to run on the Xeon-Phi:" << std::endl;
  int count = 0;
  for (node_t n: g->allNodes(1)) {
    if (n[0] != '&') {
      std::cout << "#   " << n << std::endl;
      count++;
    }
  }

  if (count == 0) {
    std::cout << "### No functions placed on the Xeon-Phi." << std::endl;
  } else {
    bool found_phi = false;
    for (node_t n: g->allNodes(1)) {
      if (NODE_T_EQ(n, xeon)) {
          std::cerr << "### ERROR: Xeon cost node mapped to Xeon-Phi."
                    << std::endl;
      } else if (NODE_T_EQ(n, phi)) {
        found_phi = true;
      }
    }
    if (!found_phi) {
      std::cerr << "### ERROR: Xeon-Phi cost node mapped to Xeon."
                << std::endl;
    }
  }

  // If a function in the Xeon-Phi partition is called from a function in the
  // Xeon Partition then it will require an explicit migration.
  std::cout << "### Functions requiring '#pragma popcorn':" << std::endl;
  for (node_t n1: g->allNodes(1)) {
    if (n1[0] == '&') continue; // Skip compute nodes.
    bool boundary_function = false;

    for (node_t n2: g->allNodes(0)) {
      if (n2[0] == '&') continue; // Skip compute nodes.
      if (fg->numCalls(n2, n1)) boundary_function = true;
    }

    if (boundary_function) std::cout << "#|  " << n1 << std::endl;
  }

  g->draw("graph.dot");

  delete g;
}

// Once the progam exits it is safe to process all the data that we have
// recorded and produce an analysis.
void ptrack_destroy(void)
{
  DEBUG(std::cerr << "*** Destroying PTrack" << std::endl);

  std::cout << "### Recorded " << fg->totalCalls() << " calls and "
            << fg->totalAccesses() << " memory accesses." << std::endl;

  ptrack_load_parallel();
  ptrack_partition();

  delete pt;
  delete fg;
}
