// test_function_graph.cpp -- Test function_graph.[cpp,h].
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include "../function_graph.h"
#include <gtest/gtest.h>

// Test a modeled call graph for correct edge values and costs.
TEST(FunctionGraphTest, Modeled) {
  const edge_t migrationCost = 1000;
  const edge_t pageFaultCost = 100;
  const edge_t computationCostArch1 = 50;
  const edge_t parallelismArch1 = 1;
  const edge_t computationCostArch2 = 200;
  const edge_t parallelismArch2 = 2;
  FunctionGraph fg(migrationCost, pageFaultCost,
                   "&A", cost_t(computationCostArch1, parallelismArch1),
                   "&B", cost_t(computationCostArch2, parallelismArch2));

  // First up, 'main' does no work, but it just calls 'foo'.  This is required
  // because the library is (sensibly) hard-coded to assume that the first
  // function is always 'main'.
  fg.call("main", "foo");

  // Call stack and the amount of work done (all on the same data).
  // foo           100
  // foo bar       100
  // foo bar baz   100
  // foo           100

  #define REPEAT(i,x) { for (int __i = 0; __i < i; __i++) { x; } }

  REPEAT(100, fg.incurPageFault("foo", "foo")); // 'foo' accesses it's own data.
  fg.call("foo", "bar");
  REPEAT(100, fg.incurPageFault("bar", "foo")); // 'bar' accesses 'foo' data.
  fg.call("bar", "baz");
  REPEAT(100, fg.incurPageFault("baz", "bar")); // 'baz' accesses 'bar' data.
  // return x 2
  REPEAT(100, fg.incurPageFault("foo", "baz")); // 'foo' accesses 'baz' data.

  #undef REPEAT

  EXPECT_EQ(fg.numCalls("foo", "foo"), 0);
  EXPECT_EQ(fg.numCalls("foo", "bar"), 1);
  EXPECT_EQ(fg.numCalls("foo", "baz"), 0);
  EXPECT_EQ(fg.numCalls("bar", "foo"), 0);
  EXPECT_EQ(fg.numCalls("bar", "bar"), 0);
  EXPECT_EQ(fg.numCalls("bar", "baz"), 1);
  EXPECT_EQ(fg.numCalls("baz", "foo"), 0);
  EXPECT_EQ(fg.numCalls("baz", "bar"), 0);
  EXPECT_EQ(fg.numCalls("baz", "baz"), 0);

  EXPECT_EQ(fg.numNonFaults("foo"), 100);
  EXPECT_EQ(fg.numNonFaults("bar"), 0);
  EXPECT_EQ(fg.numNonFaults("baz"), 0);

  EXPECT_EQ(fg.numPageFaults("foo", "bar"), 0);
  EXPECT_EQ(fg.numPageFaults("foo", "baz"), 100);
  EXPECT_EQ(fg.numPageFaults("bar", "foo"), 100);
  EXPECT_EQ(fg.numPageFaults("bar", "baz"), 0);
  EXPECT_EQ(fg.numPageFaults("baz", "foo"), 0);
  EXPECT_EQ(fg.numPageFaults("baz", "bar"), 100);

  EXPECT_EQ(fg.totalAccesses(), 400); // 100 non-faults + 300 faults.
  EXPECT_EQ(fg.totalCalls(), 3); // main->foo->bar

  EXPECT_EQ(fg.edgeCost("foo", "bar"), migrationCost);
  EXPECT_EQ(fg.edgeCost("foo", "baz"), 100*pageFaultCost);
  EXPECT_EQ(fg.edgeCost("bar", "foo"), 100*pageFaultCost);
  EXPECT_EQ(fg.edgeCost("bar", "baz"), migrationCost);
  EXPECT_EQ(fg.edgeCost("baz", "foo"), 0);
  EXPECT_EQ(fg.edgeCost("baz", "bar"), 100*pageFaultCost);

  EXPECT_EQ(fg.computationCost("foo", 0), 200*computationCostArch1);
  EXPECT_EQ(fg.computationCost("foo", 1), 200*computationCostArch2);
  EXPECT_EQ(fg.computationCost("bar", 0), 100*computationCostArch1);
  EXPECT_EQ(fg.computationCost("bar", 1), 100*computationCostArch2);
  EXPECT_EQ(fg.computationCost("baz", 0), 100*computationCostArch1);
  EXPECT_EQ(fg.computationCost("baz", 1), 100*computationCostArch2);
}
