// graph_impl.h -- Template implementations of most functions in graph.h.
//
// This file is distributed under the MIT license, see LICENSE.txt.
//
// DO NOT INCLUDE THIS FILE!  Include graph.h instead (it then includes this).

#include <iostream>
#include <fstream>
#include <cassert>

template <typename D, typename EP>
void Graph<D, EP>::addNode(node_t n)
{
  assert((node(n) == Types::Null()) && "Vertex with name already exists");
  map[n] = boost::add_vertex(VertexProperty(n), g);
}

template <typename D, typename EP>
void Graph<D, EP>::addEdge(node_t source, node_t sink, edge_t e)
{
  typename Types::V sourceV = node(source);
  typename Types::V sinkV = node(sink);
  assert((sourceV != Types::Null()) && "Vertex is not in graph");
  assert((sinkV != Types::Null()) && "Vertex is not in graph");

  typename Types::E edge; bool exists;
  boost::tie(edge, exists) = boost::edge(sourceV, sinkV, g);

  if (exists) {
    edge_t old_weight = boost::get(boost::edge_weight, g, edge);
    boost::put(boost::edge_weight, g, edge, old_weight + e);
  } else {
    boost::add_edge(sourceV, sinkV, e, g);
  }
}

template <typename D, typename EP>
std::vector<node_t> Graph<D, EP>::allNodes() const
{
  std::vector<node_t> all;
  for (auto v : Types::Vertices(g)) all.push_back(g[v].name);
  return all;
}

template <typename D, typename EP>
std::vector<node_t> Graph<D, EP>::allNodes(int partition) const
{
  std::vector<node_t> all;

  for (auto v : Types::Vertices(g)) {
    if (g[v].partition == partition) all.push_back(g[v].name);
  }

  return all;
}

template <typename D, typename EP>
edge_t Graph<D, EP>::sumEdges() const
{
  edge_t total = 0;

  for (auto e : Types::Edges(g)) {
    total += boost::get(boost::edge_weight, g, e);
  }

  return total;
}

template <typename D, typename EP>
typename Graph<D, EP>::Types::V Graph<D, EP>::node(node_t n) const
{
  auto it = map.find(n);
  if (it == map.end()) return Types::Null();
  else                 return it->second;
}

template <typename D, typename EP>
edge_t Graph<D, EP>::edge(node_t source, node_t sink) const
{
  typename Types::V sourceV = node(source);
  typename Types::V sinkV = node(sink);
  assert((sourceV != Types::Null()) && "Vertex is not in graph");
  assert((sinkV != Types::Null()) && "Vertex is not in graph");

  typename Types::E edge; bool exists;
  boost::tie(edge, exists) = boost::edge(sourceV, sinkV, g);

  if (exists) return boost::get(boost::edge_weight, g, edge);
  else        return 0;
}

template <typename D, typename EP>
bool Graph<D, EP>::exists(node_t n) const
{
  return node(n) != Types::Null();
}

// Directed graphs and undirected graphs are identical except for two small
// differences, so this function implements the general shape and takes the
// correct value for each of these small differences (graph_str and edge_str)
// as parameters.
template <typename D, typename EP>
void Graph<D, EP>::draw_implementation(const typename Types::G &g,
                                       const char *file,
                                       const char *graph_str,
                                       const char *edge_str)
{
  std::ofstream f(file);
  f << graph_str << " G {" << std::endl;

  for (auto v : Types::Vertices(g)) {
    node_t n = g[v].name;

    bool compute = (n[0] == '&');
    bool partitioned = !compute && (g[v].partition != 0);

    f << "  \"" << n << "\"";
    if (compute) f << " [shape=box, color=blue, fontcolor=blue]";
    if (partitioned) f << " [shape=box, color=red, fontcolor=red]";
    f << ";" << std::endl;
  }

  for (auto e : Types::Edges(g)) {
    typename Types::V s = boost::source(e, g);
    typename Types::V t = boost::target(e, g);
    assert((s != Types::Null()) && "Vertex is not in graph");
    assert((t != Types::Null()) && "Vertex is not in graph");

    // If either end of an edge is connected to a compute node then this is a
    // compute edge.
    bool compute = (g[s].name[0] == '&') || (g[t].name[0] == '&');

    f << "  \"" << g[s].name << "\" " << edge_str << " \"" << g[t].name
      << "\" [label=\"" << boost::get(boost::edge_weight, g, e) << "\"";
    if (compute) f << ", style=dotted, color=blue, fontcolor=blue";
    f << "];" << std::endl;
  }

  f << "}" << std::endl;
}
