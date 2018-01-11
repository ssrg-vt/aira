// function_graph.h -- Record properties about the call graph.
//
// This file is distributed under the MIT license, see LICENSE.txt.
//
// This class records which functions call which and how often, and which
// functions access pages 'owned' by other functions.  Most of this work is
// actually done by Graph (graph.h) and PageTracker (page_tracking.h), this
// class wraps these together and builds a cost model on top of them.

#ifndef _FUNCTION_GRAPH_H
#define _FUNCTION_GRAPH_H

#include <vector>
#include <map>
#include <utility>
#include "types.h"
#include "graph.h"

// Pair together compute cost and parallelism benefits for a processor type.
typedef std::pair<edge_t, edge_t> cost_t;

class FunctionGraph {
public:
  // Constructor for a two architecture system.
  FunctionGraph(edge_t m, edge_t p,
                node_t name1, cost_t arch1,
                node_t name2, cost_t arch2);

  // Whether or note a node has already been created for 'n'.
  bool nodeExists(node_t n) const;

  // If this is the first time that we have seen function 'n' then add it to
  // the various data-structures.  If we've seen it before then do nothing.
  void addNode(node_t function);

  // Mark functions as parallel.
  void setParallelFunction(node_t function, bool parallel=true);
  void loadParallelFunctions(std::ifstream &f);

  // The function 'newF' accesses a page that was most recently accessed by the
  // function 'oldF'.  Note that if 'newF == oldF' there is obviously no
  // page-fault incurred as the page is already on the correct architecture, we
  // still gather the data in case it is useful.  It has no effect on the
  // graph-cut cost as a function is trivially always on the same side of the
  // cut as itself.
  void incurPageFault(node_t newF, node_t oldF, edge_t num = 1);

  // The function 'caller' dynamically calls 'callee'. I.e. this call
  // definitely happened on this run of the program (calls that statically
  // exist but that never actually happen are not captured here).
  void call(node_t caller, node_t callee);

  // How many potential page faults occur directly from the function 'function'
  // accessing a page owned by 'old' (faults incurred via any intermediary
  // functions are not included).
  edge_t numPageFaults(node_t function, node_t old) const;

  // The number of memory accesses made by a function that are guaranteed to
  // not incur a page fault (i.e. accesses to pages that have already been
  // accessed by this function).
  edge_t numNonFaults(node_t function) const;

  // The number of times that 'caller' directly calls 'callee'.
  edge_t numCalls(node_t caller, node_t callee) const;

  // The total number of memory accesses or calls performed across the whole
  // program.  Memory accesses covers both faults and non-faults.
  edge_t totalAccesses() const;
  edge_t totalCalls() const;

  // The local cost of incurring a partitioning at the boundary between
  // 'caller' and 'callee'.  Only covers costs directly incurred by these two
  // functions, and only in the direction of caller->callee (i.e. this cost is
  // designed to be placed on the edge of a directed graph).
  edge_t edgeCost(node_t caller, node_t callee) const;

  // The cost of computing a function on a given architecture.  This will be
  // scaled according the the number of memory access.
  edge_t computationCost(node_t function, int architecture) const;

  // Build the complete cost graph that can be used for partitioning.
  UndirectedGraph* buildCostGraph() const;

private:

  // Note that all costs should be to a similar scale, cycles, nanoseconds,
  // whatever.  In particular, this means that function computational costs and
  // page fault costs should be comparable.  If they're not the eventual output
  // of the graph-cut will have little meaning.

  // The name to give the compute node of each architecture.
  std::vector<node_t> architectureNames;

  // The default computational cost for new functions on each architecture.
  std::vector<edge_t> defaultCosts;
  std::vector<edge_t> parallelism;

  // A map of whether each function is parallel or not.  Every function that
  // executes should be in the map, even if it is not parallel (value false).
  std::map<node_t, bool, NODE_T_COMPARATOR> isParallel;

  // The cost of a single cross-architecture migraton or page-fault.
  // TODO: Once there are more than two architectures this may need to be a 2D
  // matrix, but at this point (2014-07-08) it is not known how Popcorn
  // migration will look on such a system.
  const edge_t migrationCost;
  const edge_t pageFaultCost;

  // The real data is stored in these graphs, they just store the number of
  // times an event occurs (x calls y, x accesses a page owned by y), the costs
  // values above are applied once data recording has completed.
  DirectedGraph callGraph;
  DirectedGraph faultGraph;
};

#endif // _FUNCTION_GRAPH_H
