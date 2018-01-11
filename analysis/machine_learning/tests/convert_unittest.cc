#include "data.hh"
#include <gtest/gtest.h>

TEST(ConvertTest, Speedups) {
  DataManager d = load_data("tests/unittest_100x24_123.csv", 3);
  EXPECT_EQ(d.speedups(), false);
  d.convert_labels_to_speedups();
  EXPECT_EQ(d.speedups(), true);

  for (int i = 0; i < d.num_data(); i++) {
    const Row32F label = d.datapoint_labels(i);
    EXPECT_FLOAT_EQ(label.at(0), 1.0/1.0);
    EXPECT_FLOAT_EQ(label.at(1), 1.0/2.0);
    EXPECT_FLOAT_EQ(label.at(2), 1.0/3.0);
  }
}

TEST(ConvertTest, Probablities) {
  // Unlikely use-case (direct use of probabilities).
  DataManager d1 = load_data("tests/unittest_100x24_123.csv", 3);
  d1.convert_labels_to_probabilities();
  for (int i = 0; i < d1.num_data(); i++) {
    const Row32F label = d1.datapoint_labels(i);
    EXPECT_FLOAT_EQ(label.at(0), 1.0/6.0);
    EXPECT_FLOAT_EQ(label.at(1), 2.0/6.0);
    EXPECT_FLOAT_EQ(label.at(2), 3.0/6.0);
  }

  // Likely use-case (apply probabilities to speed-ups).
  DataManager d2 = load_data("tests/unittest_100x24_123.csv", 3);
  d2.convert_labels_to_speedups();
  d2.convert_labels_to_probabilities();
  for (int i = 0; i < d2.num_data(); i++) {
    const Row32F label = d2.datapoint_labels(i);
    const float sum_speedups = 1.0/1.0 + 1.0/2.0 + 1.0/3.0;
    EXPECT_FLOAT_EQ(label.at(0), (1.0/1.0)/sum_speedups);
    EXPECT_FLOAT_EQ(label.at(1), (1.0/2.0)/sum_speedups);
    EXPECT_FLOAT_EQ(label.at(2), (1.0/3.0)/sum_speedups);
  }
}

TEST(ConvertTest, Shifts) {
  DataManager d = load_data("tests/unittest_100x24_123.csv", 3);
  Mat32F data = d.features();

  Row32F rShift(data.cols());
  for (int i = 0; i < rShift.cols(); i++) {
    rShift.at(i) = i;
  }
  TransformShift shift(rShift);

  Mat32F dataS = shift.apply(data);
  for (int i = 0; i < data.rows(); i++) {
    for (int j = 0; j < data.cols(); j++) {
      EXPECT_FLOAT_EQ(data.at(i, j), dataS.at(i, j) - j);
    }
  }
}

TEST(ConvertTest, Scales) {
  DataManager d = load_data("tests/unittest_100x24_123.csv", 3);
  Mat32F data = d.features();

  Row32F rScale(data.cols());
  for (int i = 0; i < rScale.cols(); i++) {
    rScale.at(i) = i+1;
  }
  TransformScale scale(rScale);

  Mat32F dataS = scale.apply(data);
  for (int i = 0; i < data.rows(); i++) {
    for (int j = 0; j < data.cols(); j++) {
      EXPECT_FLOAT_EQ(data.at(i, j), dataS.at(i, j) / (j+1));
    }
  }
}

TEST(ConvertTest, Projections) {
  DataManager d = load_data("tests/unittest_100x24_123.csv", 3);
  Mat32F data = d.features();

  // Build a 21-dimension to 21-dimension identity matrix.
  Mat32F mIdentity(21, 21);
  for (int i = 0; i < 21; i++) {
    mIdentity.at(i,i) = 1.0;
  }
  TransformProject identity(mIdentity);
  Mat32F dataI = identity.apply(data);
  EXPECT_TRUE(data.equals(dataI));

  // Build a linear 21-dimension to 1-dimension 'sum' reduction matrix.
  Mat32F mSum(1, 21);
  for (int i = 0; i < 21; i++) {
    mSum.at(0,i) = 1.0;
  }
  TransformProject sum(mSum);
  Mat32F dataS = sum.apply(data);
  for (int i = 0; i < 21; i++) {
    EXPECT_FLOAT_EQ(data.row(i).sum(), dataS.at(i,0));
  }
}

TEST(ConvertTest, Cut) {
  DataManager d = load_data("tests/unittest_100x24_123.csv", 3);
  Mat32F data = d.features();

  TransformProjectPtr cut = BuildTransformCut(d.num_features(),
                                              {2, 5, 8, 11, 14, 17, 20});
  Mat32F dataC = cut->apply(data);
  EXPECT_EQ(dataC.rows(), data.rows());
  EXPECT_EQ(dataC.cols(), data.cols() - 7);

  for (int i = 0; i < dataC.rows(); i++) {
    for (int j = 0; j < dataC.cols(); j++) {
      EXPECT_FLOAT_EQ(dataC.at(i, j), data.at(i, j+(j/2)));
    }
  }
}

TEST(ConvertTest, CleanupProperties) {
  DataManager data = load_data("tests/unittest_100x25_linear.csv", 3);

  data.apply_scale_means();
  for (int i = 0; i < data.num_features(); i++) {
    EXPECT_FLOAT_EQ(data.feature(i).mean(), 0.0);
  }

  data.apply_scale_ranges();
  for (int i = 0; i < data.num_features(); i++) {
    EXPECT_TRUE(data.feature(i).mean() < 0.01); // May not be exactly zero.
    EXPECT_TRUE(data.feature(i).range() <= 1.0);
    EXPECT_TRUE(data.feature(i).range() >= 0.0);
  }

  data.apply_pca(8);
  EXPECT_EQ(data.num_features(), 8);
}
