// test_graph.cpp -- Test graph.[cpp, h] and graph_impl.h.
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include "../graph.h"
#include "../function_graph.h"
#include <gtest/gtest.h>
#include <cstring>

// Count the number of occurrences of 'f' in 'all'.
static int search(const std::vector<node_t> &all, node_t f) {
  int count = 0;
  for (auto i : all) if (NODE_T_EQ(i, f)) count++;
  return count;
}

// Test the properties of graphs in general, and directed graphs in particular.
TEST(GraphTest, Directed) {
  DirectedGraph g;

  // 'main' calls 'foo'.
  g.addNode("main");
  g.addNode("foo");
  g.addEdge("main", "foo", 1);

  // 'foo' calls 'bar'
  g.addNode("bar");
  g.addEdge("foo", "bar", 1);

  // 'bar' call 'foo'
  g.addEdge("bar", "foo", 1);

  // 'main' calls 'foo' again
  g.addEdge("main", "foo", 1);

  EXPECT_EQ(g.exists("main"), true);
  EXPECT_EQ(g.exists("foo"), true);
  EXPECT_EQ(g.exists("bar"), true);
  EXPECT_EQ(g.exists("baz"), false);

  // Check that we are comparing strings, not pointers.
  const char main_p[5] = {'m', 'a', 'i', 'n', '\0'};
  EXPECT_EQ(g.exists(main_p), true);

  EXPECT_EQ(g.edge("main", "foo"), 2);
  EXPECT_EQ(g.edge("main", "bar"), 0);
  EXPECT_EQ(g.edge("foo", "main"), 0);
  EXPECT_EQ(g.edge("foo", "bar"), 1);
  EXPECT_EQ(g.edge("bar", "main"), 0);
  EXPECT_EQ(g.edge("bar", "foo"), 1);

  std::vector<node_t> all = g.allNodes();
  EXPECT_EQ(search(all, "main"), 1);
  EXPECT_EQ(search(all, "foo"), 1);
  EXPECT_EQ(search(all, "bar"), 1);
  EXPECT_EQ(search(all, "baz"), 0);
}

// Only test the undirected aspects of the graph, the rest is tested above.
TEST(GraphTest, Undirected) {
  UndirectedGraph g;

  g.addNode("main");
  g.addNode("foo");
  g.addNode("bar");

  g.addEdge("main", "foo", 1);
  g.addEdge("foo", "main", 1);
  g.addEdge("foo", "bar", 1);

  EXPECT_EQ(g.edge("main", "foo"), 2);
  EXPECT_EQ(g.edge("main", "bar"), 0);
  EXPECT_EQ(g.edge("foo", "main"), 2);
  EXPECT_EQ(g.edge("foo", "bar"), 1);
  EXPECT_EQ(g.edge("bar", "main"), 0);
  EXPECT_EQ(g.edge("bar", "foo"), 1);
  EXPECT_EQ(g.sumEdges(), 3);

  std::vector<node_t> all = g.allNodes();
  EXPECT_EQ(search(all, "main"), 1);
  EXPECT_EQ(search(all, "foo"), 1);
  EXPECT_EQ(search(all, "bar"), 1);
  EXPECT_EQ(search(all, "baz"), 0);
}

// A test graph for the partitioning algorithms.
//
// Test case courtesy of 'dingalapadum' on Stack Overflow:
//   https://stackoverflow.com/a/21219223
//
// All edges have cost 1, the global minimum cut is {A,B,C,D,E} & {F,G,H,I,J}
// (i.e. across the edge E-F, so it has a cost of 1).
//
//      B        G
//     / \      / \
//    /   \    /   \
//   A--D--E--F--I--J
//    \   /    \   /
//     \ /      \ /
//      C        H
//
static UndirectedGraph* testGraph()
{
  UndirectedGraph *g = new UndirectedGraph();

  g->addNode("A");
  g->addNode("B");
  g->addNode("C");
  g->addNode("D");
  g->addNode("E");
  g->addNode("F");
  g->addNode("G");
  g->addNode("H");
  g->addNode("I");
  g->addNode("J");

  // Left cluster
  g->addEdge("A", "B", 1);
  g->addEdge("A", "C", 1);
  g->addEdge("A", "D", 1);
  g->addEdge("D", "E", 1);
  g->addEdge("B", "E", 1);
  g->addEdge("C", "E", 1);

  // Join
  g->addEdge("E", "F", 1);

  // Right cluster
  g->addEdge("F", "G", 1);
  g->addEdge("F", "H", 1);
  g->addEdge("F", "I", 1);
  g->addEdge("I", "J", 1);
  g->addEdge("G", "J", 1);
  g->addEdge("H", "J", 1);

  return g;
}

// First test the global cut, i.e. the minimum cut for all pairs of vertices.
TEST(GraphTest, GlobalPartitioning) {
  UndirectedGraph *g = testGraph();

  EXPECT_EQ(g->partition(), 1);
  EXPECT_EQ(g->allNodes(0).size(), 5);
  EXPECT_EQ(g->allNodes(1).size(), 5);
  std::vector<node_t> left = g->allNodes(0);
  std::vector<node_t> right = g->allNodes(1);
  EXPECT_EQ(left.size(), 5);
  EXPECT_EQ(right.size(), 5);
  EXPECT_EQ(search(left, "A"), 1);
  EXPECT_EQ(search(left, "B"), 1);
  EXPECT_EQ(search(left, "C"), 1);
  EXPECT_EQ(search(left, "D"), 1);
  EXPECT_EQ(search(left, "E"), 1);
  EXPECT_EQ(search(right, "F"), 1);
  EXPECT_EQ(search(right, "G"), 1);
  EXPECT_EQ(search(right, "H"), 1);
  EXPECT_EQ(search(right, "I"), 1);
  EXPECT_EQ(search(right, "J"), 1);
  
  delete g;
}

// Now test the s-t minimum cut for across the two clusters.
TEST(GraphTest, STPartitioning) {
  UndirectedGraph *g = testGraph();

  EXPECT_EQ(g->partition("A", "J"), 1);
  std::vector<node_t> left = g->allNodes(0);
  std::vector<node_t> right = g->allNodes(1);
  EXPECT_EQ(left.size(), 5);
  EXPECT_EQ(right.size(), 5);
  EXPECT_EQ(search(left, "A"), 1);
  EXPECT_EQ(search(left, "B"), 1);
  EXPECT_EQ(search(left, "C"), 1);
  EXPECT_EQ(search(left, "D"), 1);
  EXPECT_EQ(search(left, "E"), 1);
  EXPECT_EQ(search(right, "F"), 1);
  EXPECT_EQ(search(right, "G"), 1);
  EXPECT_EQ(search(right, "H"), 1);
  EXPECT_EQ(search(right, "I"), 1);
  EXPECT_EQ(search(right, "J"), 1);

  delete g;
}

// Finally, test the s-t minimum cut *within* one of clusters.  testGraph has
// two equal B/C partitionings, so we bias one of those with an edge of weight
// 2.  The correct cut is {B} & {A,C,D,E,F,G,H,I,J}.
TEST(GraphTest, STPartitioning2) {
  UndirectedGraph *g = testGraph();
  g->addEdge("A", "C", 1);  // Edge with weight 2.

  EXPECT_EQ(g->partition("B", "C"), 2);
  std::vector<node_t> left = g->allNodes(0);
  std::vector<node_t> right = g->allNodes(1);
  EXPECT_EQ(left.size(), 1);
  EXPECT_EQ(right.size(), 9);
  EXPECT_EQ(search(right, "A"), 1);
  EXPECT_EQ(search(left, "B"), 1);
  EXPECT_EQ(search(right, "C"), 1);
  EXPECT_EQ(search(right, "D"), 1);
  EXPECT_EQ(search(right, "E"), 1);
  EXPECT_EQ(search(right, "F"), 1);
  EXPECT_EQ(search(right, "G"), 1);
  EXPECT_EQ(search(right, "H"), 1);
  EXPECT_EQ(search(right, "I"), 1);
  EXPECT_EQ(search(right, "J"), 1);

  delete g;
}
