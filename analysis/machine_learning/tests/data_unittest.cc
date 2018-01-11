#include "data.hh"
#include <gtest/gtest.h>
#include <vector>
#include <string>

TEST(DataTest, BuildProperties) {
  DataManager d1 = load_data("tests/unittest_100x25_linear.csv", 5);
  DataManager d2 = load_data("tests/unittest_100x25_linear.csv", 3);
  std::vector<std::string> files = { "tests/unittest_3x3_123.csv",
                                     "tests/unittest_3x3_linear.csv" };
  DataManager d3 = load_data(files, 1);

  EXPECT_EQ(d1.num_data(), 100);
  EXPECT_EQ(d1.num_features(), 20);
  EXPECT_EQ(d1.num_labels(), 5);

  EXPECT_EQ(d2.num_data(), 100);
  EXPECT_EQ(d2.num_features(), 22);
  EXPECT_EQ(d2.num_labels(), 3);

  EXPECT_EQ(d3.num_data(), 6);
  EXPECT_EQ(d3.num_features(), 2);
  EXPECT_EQ(d3.num_labels(), 1);
}

// The accessors of DataManager are trivial wrappers around Mat32F accessors,
// so only lightly test them.
TEST(DataTest, AccessorProperties) {
  DataManager d = load_data("tests/unittest_100x25_linear.csv", 5);

  EXPECT_EQ(d.datapoint_features(0).cols(), 20);
  EXPECT_EQ(d.datapoint_labels(0).cols(), 5);

  EXPECT_EQ(d.feature(0).rows(), 100);
  EXPECT_EQ(d.label(0).rows(), 100);
}

TEST(DataTest, Classify) {
  DataManager d = load_data("tests/unittest_100x23_123.csv", 3);
  Col32D smallest = d.classifications(); // Index of smallest labels

  EXPECT_EQ(d.num_data(), smallest.rows());

  int expected = 1;
  for (int i = 0; i < d.num_data(); i++) {
    EXPECT_EQ(smallest.at(i), expected);
    expected = (expected + 1) % 3;
  }
}
