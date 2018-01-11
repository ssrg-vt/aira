// function_graph.cpp -- See function_graph.h.
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include "function_graph.h"

// Constructor for a two architecture system, if three architecture systems are
// required then this constructor can be copy-pasted-modified in the obvious
// manner.  Note that the partioning algorithms in graph.cpp all find binary
// min-cuts (i.e. partitioning for two architectures).
FunctionGraph::FunctionGraph(edge_t m, edge_t p,
                             node_t name1, cost_t arch1,
                             node_t name2, cost_t arch2)
  : architectureNames(2), defaultCosts(2), parallelism(2),
    migrationCost(m), pageFaultCost(p)
{
  assert((name1[0] == '&') && "Invalid compute node name");
  assert((name2[0] == '&') && "Invalid compute node name");
  architectureNames.at(0) = name1;
  architectureNames.at(1) = name2;

  defaultCosts.at(0) = arch1.first;
  defaultCosts.at(1) = arch2.first;

  parallelism.at(0) = arch1.second;
  parallelism.at(1) = arch2.second;

  // Nothing calls main, but it exists!  So manually add the node.
  addNode("main");
}

bool FunctionGraph::nodeExists(node_t n) const
{
  return isParallel.find(n) != isParallel.end();
}

void FunctionGraph::addNode(node_t function)
{
  // Only add a node if it does not already exist.
  if (!nodeExists(function)) {
    isParallel[function] = false;
    callGraph.addNode(function);
    faultGraph.addNode(function);
  }
}

void FunctionGraph::setParallelFunction(node_t function, bool parallel)
{
  // If we have no data on 'function' by now there is no point creating the
  // node.  It will nearly always exist by the time this function is called,
  // the reason it might not is because 'function' came from the list of
  // parallel functions, but for some reason 'function' was never executed.
  // The most obvious reason is that 'function' is dead code, another reason is
  // that 'function' has been inlined and thus does not exist (this, however,
  // should be avoided because it makes it harder to relate the final result
  // back to the source).

  // If 'function' doesn't exist or is already set correctly then we're done.
  if (!nodeExists(function)) return;
  if (isParallel.at(function) == parallel) return;

  isParallel[function] = parallel;

  // If a function is parallel, then any function that it calls will be used in
  // parallel, even if that function is not parallel itself.
  // TODO: Only propagate parallelism to functions that are actually called
  // from parallel regions, rather than anywhere in a function containing a
  // parallel region.  The library currently doesn't have enough information to
  // know the difference.
  if (parallel) {
    for (auto n: callGraph.allNodes()) {
      if (callGraph.edge(function, n) > 0) setParallelFunction(n);
    }
  }
}

void FunctionGraph::loadParallelFunctions(std::ifstream &f)
{
  while (!f.eof()) {
    char buf[255];
    f.getline(buf, 255);

    // TODO: Don't leak memory.  This isn't a bad leak, it can't be any bigger
    // than the parallelism file, and the data is required until almost the end
    // of execution.  The reason it is non-trivial to fix is because isParallel
    // contains a mix of pointers to global values, and malloc'ed values.
    if (strlen(buf) > 0) setParallelFunction(strndup(buf, 255));
  }
}

void FunctionGraph::incurPageFault(node_t newF, node_t oldF, edge_t num)
{
  // The node for newF has nearly always been created, so it is a pain that we
  // have to check that.  The situtation where it doesn't exist occurs when
  // newF was called via a function pointer (and thus the LLVM annotator was
  // not able to track what function was called).  Once it starts running it
  // will incur a page fault, but we have no record of the function having ever
  // been called, so we create the node now (though we still don't know who
  // called it).  oldF MUST already exist, or how else do we know that it is
  // the current owner the page.
  addNode(newF);
  assert(nodeExists(oldF) && "Impossible page fault");

  faultGraph.addEdge(newF, oldF, num);
}

void FunctionGraph::call(node_t caller, node_t callee)
{
  // Again, 'caller' nearly always already exists (see incurPageFault comment),
  // 'callee' however will frequently not exist yet (i.e. the situtation where
  // this is the first time that 'callee' has been called).
  addNode(caller);
  addNode(callee);

  callGraph.addEdge(caller, callee, 1);
}

edge_t FunctionGraph::numPageFaults(node_t function, node_t old) const
{
  assert(nodeExists(function) && "Trying to get faults for invalid function");
  assert(nodeExists(old) && "Trying to get faults for invalid function");

  return faultGraph.edge(function, old);
}

edge_t FunctionGraph::numNonFaults(node_t function) const
{
  assert(nodeExists(function) && "Trying to get faults for invalid function");

  return numPageFaults(function, function);
}

edge_t FunctionGraph::numCalls(node_t caller, node_t callee) const
{
  assert(nodeExists(caller) && "Trying to get edge cost with invalid node");
  assert(nodeExists(callee) && "Trying to get edge cost with invalid node");

  return callGraph.edge(caller, callee);
}

edge_t FunctionGraph::totalAccesses() const
{
  return faultGraph.sumEdges();
}

edge_t FunctionGraph::totalCalls() const
{
  return callGraph.sumEdges();
}

edge_t FunctionGraph::edgeCost(node_t caller, node_t callee) const
{
  assert(nodeExists(caller) && "Trying to get edge cost with invalid node");
  assert(nodeExists(callee) && "Trying to get edge cost with invalid node");

  // Represent the raw cost of migrating a program from kernel 0 to kernel 1
  // (or vice-versa).  Not considering it may cause the library to considering
  // ping-ponging migrations to be acceptable.
  edge_t migrations = numCalls(caller, callee);

  // Represent the cost of moving data around (probably the most important cost
  // of all).
  edge_t faults = numPageFaults(caller, callee);

  // Weight the raw values by their associated costs.
  edge_t mCost = migrations * migrationCost;
  edge_t pCost = faults * pageFaultCost;

  return mCost + pCost;
}

// Scale default function costs by the number of page accesses (including
// access to pages already owned by that function) to give a vague difference
// between functions that take 5 cycles and functions that are long running
// loops.  Obviously a gross simplification, but roughly speaking a function
// rarely spends a long time computing without read/writing memory (if a
// function does do this it implies a long iterative calculation based on
// register values only).
edge_t FunctionGraph::computationCost(node_t function, int architecture) const
{
  assert(((architecture >= 0) && (architecture < defaultCosts.size()))
         && "Invalid architecture");
  assert(nodeExists(function) && "Trying to get cost of invalid node");

  edge_t faults = 0;
  for (auto n: faultGraph.allNodes()) faults += faultGraph.edge(function, n);

  edge_t cost = faults * defaultCosts[architecture];

  // If a function is parallel, then we can divide it's computation cost, if
  // it's not, we can't.
  edge_t parallel = isParallel.at(function) ? parallelism[architecture] : 1;

  return cost / parallel;
}

// Take the two directed graphs (callGraph and faultGraph) and produce a single
// undirected cost graph.
UndirectedGraph* FunctionGraph::buildCostGraph() const
{
  auto *g = new UndirectedGraph();
  std::vector<node_t> all = callGraph.allNodes();

  // The new graph contains all the same vertices as the existing graphs.
  for (auto n : all) {
    g->addNode(n);
  }

  // Calculate the cost for every pair of nodes (many pairs will have a 0 cost,
  // i.e. no edge required).  Because the source graphs are directed we should
  // consider both a->b and b->a, even if the produced graph only has a single
  // a<->b edge (the two edges will get combined automatically). 
  for (auto n1 : all) {
    for (auto n2 : all) {
      if (NODE_T_EQ(n1, n2)) continue;

      edge_t cost = edgeCost(n1, n2);
      if (cost > 0) {
        g->addEdge(n1, n2, cost);
      }
    }
  }

  // Now we add in the computation cost by adding a fake node per-architecture,
  // and connecting every single vertex to each of these fake nodes with a
  // compute cost.

  // Add a fake compute node for each architecture.
  for (auto a: architectureNames) {
    g->addNode(a);
  }

  // TODO: These costs only make sense for a two architecture system.
  for (auto n : all) {
    // The architecture names and number are mismatched on purpose!  This is a
    // cost, so if something runs well on Xeon and badly on Xeon-Phi it should
    // have a high-weight on Xeon and a low-weight on Xeon-Phi.  Because we're
    // looking for graph cuts the weight is essentially the cost of *not*
    // running on that architecture.
    g->addEdge(architectureNames[0], n, computationCost(n, 1));
    g->addEdge(n, architectureNames[1], computationCost(n, 0));
  }

  // Add an enormous computation weight for 'main' on any non-primary
  // architectures as on Popcorn it can only legally be placed on the host
  // architecture.  This weight means that no (sensible) graph-cut would ever
  // place the main function on any architecture other than the first.
  g->addEdge(architectureNames[0], "main", ((edge_t)1) << 60);

  return g;
}
