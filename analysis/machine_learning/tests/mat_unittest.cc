#include "mat.hh"
#include <gtest/gtest.h>

// Test the basic properties of the matrix, this is all pretty obvious, perhaps
// too obvious to be worth testing -- but this is my first time using "gtest",
// lets have some obviously correct tests to play with!
TEST(MatTest, ConstructionProperties) {
  const Mat32F m1(10, 5);
  const Mat64F m2(20, 10);
  
  // Dimensions
  EXPECT_EQ(m1.rows(), 10);
  EXPECT_EQ(m1.cols(), 5);

  // Check zero-init
  EXPECT_EQ(m1.at(0, 0), 0.0);
  EXPECT_EQ(m1.at(9, 4), 0.0);
  EXPECT_EQ(m2.at(0, 0), 0.0);
  EXPECT_EQ(m2.at(19, 9), 0.0);

  // Ensure we can't access invalid entries.
  // TODO: These result in annoying threading warnings, how to eliminate?
  EXPECT_DEATH(m1.at(10,0), "Invalid row index.");
  EXPECT_DEATH(m1.at(0,5), "Invalid col index.");
}

TEST(MatTest, SubtypeConstructionProperties) {
  // Really these are very simple wrappers around Mat[32,64]F, so all the test
  // of that covers these types.  But as they are templates it is nice to make
  // sure that they can be instantiated etc.
  const Row32F r1(10);
  const Row64F r2(10);
  const Col32F c1(5);
  const Col64F c2(5);

  // Dimensions
  EXPECT_EQ(r1.rows(), 1);
  EXPECT_EQ(r1.cols(), 10);
  EXPECT_EQ(r2.rows(), 1);
  EXPECT_EQ(r2.cols(), 10);
  EXPECT_EQ(c1.rows(), 5);
  EXPECT_EQ(c1.cols(), 1);
  EXPECT_EQ(c2.rows(), 5);
  EXPECT_EQ(c2.cols(), 1);
}

#define VAL(IND_I, IND_J) ((IND_I)*3 + IND_J + 1)

// Loading data from CSV is our standard use case.
TEST(MatTest, Loading) {
  const Mat32F m = load_csv32("tests/unittest_3x3_linear.csv");

  EXPECT_EQ(m.rows(), 3);
  EXPECT_EQ(m.cols(), 3);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      EXPECT_EQ(m.at(i, j), (float)VAL(i, j));
    }
  }
}

TEST(MatTest, Equals) {
  const Mat32F m1 = load_csv32("tests/unittest_3x3_linear.csv");
  const Mat32F m2 = load_csv32("tests/unittest_3x3_123.csv");
  EXPECT_TRUE(m1.equals(m1));
  EXPECT_TRUE(m2.equals(m2));
  EXPECT_FALSE(m1.equals(m2));
  EXPECT_FALSE(m2.equals(m1));

  Mat32F identity(3, 3);
  for (int i = 0; i < 3; i++) {
    identity.at(i,i) = 1.0;
  }

  const Mat32F m1i = m1 * identity;
  const Mat32F m2i = m2 * identity;
  EXPECT_TRUE(m1.equals(m1i));
  EXPECT_TRUE(m2.equals(m2i));
  EXPECT_FALSE(m1i.equals(m2i));
  EXPECT_FALSE(m2i.equals(m1i));
}

TEST(MatTest, Concat) {
  const Mat32F m1(10,5);
  const Mat32F m2(20,5);
  const Mat32F m3(10,3);

  const Mat32F vertical = m1.concat_vertical(m2);
  EXPECT_EQ(vertical.rows(), 30);
  EXPECT_EQ(vertical.cols(), 5);

  const Mat32F horizontal = m1.concat_horizontal(m3);
  EXPECT_EQ(horizontal.rows(), 10);
  EXPECT_EQ(horizontal.cols(), 8);
}

// The split function splits a matrix into two (column-wise, i.e. the number of
// rows is unchanged).
TEST(MatTest, Split) {
  const Mat32F m1(10, 5);
  std::pair<const Mat32F, const Mat32F> s1 = m1.split(3);
  EXPECT_EQ(s1.first.rows(), 10);
  EXPECT_EQ(s1.first.cols(), 3);
  EXPECT_EQ(s1.second.rows(), 10);
  EXPECT_EQ(s1.second.cols(), 2);

  const Mat32F m2 = load_csv32("tests/unittest_3x3_linear.csv");
  std::pair<const Mat32F, const Mat32F> s2 = m2.split(2);
  EXPECT_EQ(s2.first.rows(), 3);
  EXPECT_EQ(s2.first.cols(), 2);
  EXPECT_EQ(s2.second.rows(), 3);
  EXPECT_EQ(s2.second.cols(), 1);
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(s2.first.at(i, 0), (float)VAL(i, 0));
    EXPECT_EQ(s2.first.at(i, 1), (float)VAL(i, 1));
    EXPECT_EQ(s2.second.at(i, 0), (float)VAL(i, 2));
  }
}

TEST(MatTest, Mutability) {
  Mat32F m(3, 3);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      m.at(i,j) = (float)VAL(i, j);
    }
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      EXPECT_EQ(m.at(i, j), (float)VAL(i, j));
    }
  }
}

TEST(MatTest, SubTypeAccess) {
  const Mat32F m1 = load_csv32("tests/unittest_3x3_linear.csv");

  const Row32F r1 = m1.row(1);
  const Col32F c1 = m1.col(1);

  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(r1.at(i), m1.at(1,i));
    EXPECT_EQ(c1.at(i), m1.at(i,1));
  }
}

TEST(MatTest, RowNesting) {
  const Mat32F m = load_csv32("tests/unittest_3x3_linear.csv");
  Mat32F m1(3,3); // Will be a copy of m (row-by-row).

  for (int i = 0; i < 3; i++) {
    const Row32F r = m.row(i); // Points to the same backing data as m.
    Row32F r1 = m1.row(i); // Points to the same backing data as m1.

    for (int j = 0; j < 3; j++) {
      r1.at(j) = r.at(j);
    }
  }

  for (int i = 0; i < 3; i++) {
    const Row32F r1 = m1.row(i);

    for (int j = 0; j < 3; j++) {
      EXPECT_EQ(m.at(i,j), m1.at(i,j));
      EXPECT_EQ(m.at(i,j), r1.at(j));
    }
  }
}

TEST(MatTest, ColumnNesting) {
  const Mat32F m = load_csv32("tests/unittest_3x3_linear.csv");
  Mat32F m1(3,3); // Will be the transpose of m.

  for (int i = 0; i < 3; i++) {
    const Row32F r = m.row(i); // Points to the same backing data as m.
    Col32F col = m1.col(i); // Points to the same backing data as m1.

    for (int j = 0; j < 3; j++) {
      col.at(j) = r.at(j);
    }
  }

  for (int i = 0; i < 3; i++) {
    const Col32F col = m1.col(i);

    for (int j = 0; j < 3; j++) {
      EXPECT_EQ(m.at(i,j), m1.at(j,i)); // Swap indices for transpose.
      EXPECT_EQ(m.at(i,j), col.at(j)); // Column implicity swaps indices.
    }
  }
}

TEST(MatTest, ArithmeticProperties) {
  const Mat32F linear = load_csv32("tests/unittest_3x3_linear.csv");
  const Mat32F bounded = load_csv32("tests/unittest_3x3_123.csv");
  const Mat32F negative = Mat32F(3,3) - bounded;

  EXPECT_FLOAT_EQ(linear.sum(), 45.0);
  EXPECT_FLOAT_EQ(bounded.sum(), 18.0);

  EXPECT_FLOAT_EQ(linear.mean(), 45.0 / 9.0);
  EXPECT_FLOAT_EQ(bounded.mean(), 18.0 / 9.0);
  
  // Calculating mean is a case where forcing 64-bit calculations seems
  // sensible, as intermediate values (i.e. the sum) can be huge.
  EXPECT_FLOAT_EQ(linear.convert64().mean(), 45.0 / 9.0);
  EXPECT_FLOAT_EQ(bounded.convert64().mean(), 18.0 / 9.0);

  EXPECT_FLOAT_EQ(linear.min(), 1.0);
  EXPECT_FLOAT_EQ(bounded.min(), 1.0);
  EXPECT_FLOAT_EQ(negative.min(), -3.0);

  EXPECT_FLOAT_EQ(linear.max(), 9.0);
  EXPECT_FLOAT_EQ(bounded.max(), 3.0);
  EXPECT_FLOAT_EQ(negative.max(), -1.0);

  EXPECT_FLOAT_EQ(linear.range(), 8.0);
  EXPECT_FLOAT_EQ(bounded.range(), 2.0);
  EXPECT_FLOAT_EQ(negative.range(), 2.0);
}

TEST(MatTest, Conversion) {
  const Mat32F m32 = load_csv32("tests/unittest_3x3_linear.csv");
  const Mat64F m64 = m32.convert64();
  const Mat32F m32b = m64.convert32();

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      EXPECT_FLOAT_EQ(m32.at(i, j), m64.at(i, j));
      EXPECT_FLOAT_EQ(m32.at(i, j), m32b.at(i, j));
    }
  }
}
