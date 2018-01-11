// graph.h -- Provide access to requires Boost Graph Library Functionality.
//
// This file is distributed under the MIT license, see LICENSE.txt.
//
// To avoid the need to use the Boost Graph Library directly in the main code
// we have this abstraction layer.  It provides graphs where each vertex can
// have a name and a partition assignment, edge properties are templated.
//
// The main purpose of Graph<> is to provide a way to record edge properties
//
// Not all functions make sense for all template types, this should only be a
// "problem" for FlowNetworks, but it's not actually a problem because those
// are only used in one place in a very specific way.  Partitioning (global
// min-cut or s-t min-cut) is only implemented for UndirectedGraph, it is
// possible to implement it for DirectedGraph but the implementation will be
// different and I never had the need.
//
// Most functions are implemented in graph_impl.h, but a few functions which
// have per-template specialisations are implemented in graph.cpp.
//
// As Boost is massively templated this class is lightly templated, however at
// the very bottom of this file three different template combinations
// typedef'd, these represent the only current use cases of the Graph<> class.

#ifndef _GRAPH_H
#define _GRAPH_H

#include <map>
#include <vector>
#include <utility>
#include <boost/graph/adjacency_list.hpp>

#include "types.h"

template <typename D, typename EP> // Direction, EdgeProperty
class Graph {
public:

  // All the properties that we store per-vertex.
  struct VertexProperty {
    node_t name;
    int partition;

    VertexProperty() : name("unnamed"), partition(0) {}
    VertexProperty(node_t _name) : name(_name), partition(0) {}

    bool operator==(node_t other) const { return NODE_T_EQ(name, other); }
  };

  // A wrapper class for various boost types or helper functions given more
  // convient names.
  class Types {
  public:
    typedef typename boost::adjacency_list<boost::vecS, boost::vecS, D,
                                           VertexProperty, EP> G;
    typedef typename boost::graph_traits<G>::vertex_descriptor V;
    typedef typename boost::graph_traits<G>::edge_descriptor   E;
    typedef typename boost::graph_traits<G>::vertex_iterator   VI;
    typedef typename boost::graph_traits<G>::edge_iterator     EI;
    typedef typename boost::graph_traits<G>::out_edge_iterator OEI;

    static const V Null() { return G::null_vertex(); }

    static boost::iterator_range<VI> Vertices(const G &g) {
      std::pair<VI, VI> v = boost::vertices(g);
      return boost::make_iterator_range(v.first, v.second);
    }

    static boost::iterator_range<EI> Edges(const G &g) {
      std::pair<EI, EI> e = boost::edges(g);
      return boost::make_iterator_range(e.first, e.second);
    }

    static boost::iterator_range<OEI> OutEdges(const V &v, const G &g) {
      std::pair<OEI, OEI> e = boost::out_edges(v, g);
      return boost::make_iterator_range(e.first, e.second);
    }
  };

  void addNode(node_t n);
  void addEdge(node_t source, node_t sink, edge_t e);

  // Return a vector containing the names associated with every vertex in the
  // graph, or every vertex belonging to a particular partition.
  std::vector<node_t> allNodes() const;
  std::vector<node_t> allNodes(int partition) const;

  // The sum of all edges in the graph.
  edge_t sumEdges() const;

  // Return the weight associated with the edge between two nodes.  This
  // function only makes sense for template types which use edge_weight as
  // their edge property (EP in the template).
  edge_t edge(node_t source, node_t sink) const;

  // Whether or not there exists a vertex with the given name property.
  bool exists(node_t n) const;

  // Partition the graph based on min-cut, or s-t min-cut.  The former is
  // computed directly, the latter coverts to a network flow graph, solves for
  // max-flow and then uses max-flow/min-cut duality to pick a partitioning.
  // Note: These functions are only implemented for undirected graphs.
  edge_t partition();
  edge_t partition(node_t s, node_t t);

  // Output a DOT graph representing all data contained within.  Because we
  // need to output (slightly) different DOT graphs for directed or undirected
  // graphs we make use of the type system to pick the correct implementation.
  void draw(const char *file) const;
  static void draw_implementation(const typename Types::G &g, const char *file,
                                  const char *graph_str, const char *edge_str);

private:
  // The actual boost graph.
  typename Types::G g;

  // Boost accesses vertices via numbers, but we want to look them up by name.
  // So we have a map of names to vertex numbers so that we can quickly find
  // the correct vertex.  The labeled_graph type in Boost could actually do
  // this work for us, but it is very poorly supported compared to the other
  // graph types.
  std::map<node_t, typename Types::V, NODE_T_COMPARATOR> map;
  typename Types::V node(node_t n) const;
};

// Define some 'simple' weighted graphs, directed and undirected.
typedef boost::property<boost::edge_weight_t, edge_t> Weight;
typedef Graph<boost::directedS, Weight> DirectedGraph;
typedef Graph<boost::undirectedS, Weight> UndirectedGraph;

// Define a flow-network graph.  This is more complicated as Boost requires
// that each edge has a capacity and a residual capacity, and that every edge
// should (a) have a reverse edge, and (b) that every edge should know which
// edge is its reverse.
typedef boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                     boost::directedS> FlowTrait;
typedef boost::property<boost::edge_capacity_t, edge_t,
          boost::property<boost::edge_residual_capacity_t, edge_t,
          boost::property<boost::edge_reverse_t,
                          FlowTrait::edge_descriptor>>> Flow;
typedef Graph<boost::directedS, Flow> FlowNetwork;

#include "graph_impl.h"

#endif // _GRAPH_H
