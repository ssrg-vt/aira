// graph.cpp -- See graph.h and graph_impl.h.
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include <boost/graph/one_bit_color_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/graph/edmonds_karp_max_flow.hpp>

#include "graph.h"

// Use Boost's Stoer Wagner Min-Cut algorithm to find the global minimum cut on an undirected graph.
template <>
edge_t UndirectedGraph::partition()
{
  // Boost types can be quite a pain, so let's just use the magic of type
  // inference rather than typing out typenames that don't even fit on a line.

  // Boost's implementation of this algorithm stores the cut (i.e. the
  // partitioning) in a parity map, so create one for it to use.  One bit per
  // node, to represent which side of the cut that node is on.
  auto num_vertices = boost::num_vertices(g);
  auto vertices = boost::get(boost::vertex_index, g);
  BOOST_AUTO(parities, boost::make_one_bit_color_map(num_vertices, vertices));

  // Perform the main calculation.
  auto weight_map = boost::get(boost::edge_weight, g);
  auto parity_map = boost::parity_map(parities);
  edge_t w = boost::stoer_wagner_min_cut(g, weight_map, parity_map);

  // Mark the found partition on the graph's vertex properties.
  for (auto v: Types::Vertices(g)) {
    g[v].partition = boost::get(parities, v);
  }

  return w;
}

// Perform a simple DFS and set the partition on all reachable nodes.
// NOTE: Here we could create a filtered_graph of all edges in the residual
// network with remaining capacity, and then do a reachability search on that
// using boost, but this approach is simpler.
static void partitionReachable(FlowNetwork::Types::G &g,
                               FlowNetwork::Types::V &start,
                               int partition)
{
  // Avoid infinite loops on cycles, stop at already processed nodes.
  if (g[start].partition == partition) return;

  // We've reached this vertex, so mark it.
  g[start].partition = partition;

  // Now visit all neighbours that we can reach via residual flow capacity.
  for (auto e : FlowNetwork::Types::OutEdges(start, g)) {
    if (boost::get(boost::edge_residual_capacity, g, e) > 0) {
      FlowNetwork::Types::V target = boost::target(e, g);
      partitionReachable(g, target, partition);
    }
  }
}

// This function finds an s-t min-cut, i.e. the min-cut that also satisfies the
// requirement of having the nodes 's' and 't' on opposite sides of the cut.
//
// This is not implemented directly, but via the max-flow/min-cut duality.
// What this means is that we map this graph to a flow network, solve for
// max-flow, and then map that flow back to a min-cut.
template <>
edge_t UndirectedGraph::partition(node_t source, node_t sink)
{
  Types::V sourceV = node(source);
  Types::V sinkV = node(sink);
  assert(sourceV != Types::Null());
  assert(sinkV != Types::Null());

  using namespace boost;

  FlowNetwork::Types::G fg;

  // The max-flow algorithm needs to know which pairs of edges are the reverse
  // of each other, so we have a map to store that property.
  auto reverse = get(edge_reverse, fg);

  // Now we convert this undirected graph to a flow network.  Firstly, the flow
  // network is directed, so for every edge a--b in the original graph  we
  // create an edge a->b, and one b->a, each with the same weight as a--b.
  // Creating edges as pairs like this also makes it trivial to build the
  // required reverse-edge map (each edge in a pair is the reverse of the
  // other).
  for (auto e : Types::Edges(g)) {
    // This probably shouldn't assume that a vertex_descriptor from an edge in
    // 'g' has a one-to-one map with a 'fg' vertex_descriptor, but it works.

    Types::V s = boost::source(e, g);
    Types::V t = boost::target(e, g);
    edge_t weight = get(edge_weight, g, e);

    FlowNetwork::Types::E e1, e2;
    bool inserted;

    tie(e1, inserted) = add_edge(s, t, weight, fg);
    tie(e2, inserted) = add_edge(t, s, weight, fg);
    reverse[e1] = e2;
    reverse[e2] = e1;
  }

  // Run the max-flow algorithm.
  edge_t flow = edmonds_karp_max_flow(fg, sourceV, sinkV);

  // We now have a max-flow of the flow network.  To map that back to a min-cut
  // of the original graph we take the residual network (i.e. every edge which
  // is not at maximum capacity in the max-flow) and perform a reachability
  // search from the original source node.  Every node that we can reach goes
  // in one partition, every node that we can't goes in another.  If this seems
  // like voodoo search the internet for "max-flow/min-cut duality", there is
  // plenty written on the topic.

  // We set all functions to Xeon-Phi, but then we do a reachability search
  // from the source node and mark all reachable nodes as Xeon again.
  for (auto v: FlowNetwork::Types::Vertices(fg)) {
    fg[v].partition = 1;
  }

  partitionReachable(fg, sourceV, 0);

  // Now we copy the flow graph partitioning back into the original graph.
  for (auto v: FlowNetwork::Types::Vertices(fg)) {
    g[v].partition = fg[v].partition;
  }

  return flow;
}

template <>
void DirectedGraph::draw(const char *file) const
{
  draw_implementation(g, file, "digraph", "-->");
}

template <>
void UndirectedGraph::draw(const char *file) const
{
  draw_implementation(g, file, "graph", "--");
}
