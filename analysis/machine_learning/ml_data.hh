#ifndef _ML_DATA_HH
#define _ML_DATA_HH

#include <iostream>
#include <opencv/cv.h>
#include <opencv/ml.h>
using namespace cv;

class Transform {
public:
  const int input_dimension;
  const int output_dimension;

private:
  const Mat _include; // Vector of booleans, include this feature or not
  const Mat _mean_adjust; // Value to substract to get zero-mean
  const Mat _range_scale; // Value to divide by to get constant range
  const Mat _projection; // rows: output-dimension, cols: input-dimension

public:
  Transform(int i, int o, const Mat &inc, const Mat &m, const Mat &r,
            const Mat &p) : input_dimension(i), output_dimension(o),
                            _include(inc), _mean_adjust(m), _range_scale(r),
                            _projection(p)
  {
    assert(inc.type() == CV_8U);
    assert(inc.rows == 1);
    assert(inc.cols == i);
    assert(m.type() == CV_64F);
    assert(m.rows == 1);
    assert(r.type() == CV_64F);
    assert(r.rows == 1);
    assert(p.type() == CV_64F);
    assert(p.cols == m.cols); // We have a mean-adjust for all input-features.
    assert(p.cols == r.cols); // We have a range-scale for all input-features.
    assert(p.rows == o);
  }
  ~Transform() {}

  // Save to an XML file.
  void save(const char *fname) const;

  // Don't use the "return Mat" structure used everywhere in this code as this
  // function must be extremely fast.  So use a reference.
  void project(const Mat &in, Mat &out) const;

  const Mat& include() const { return _include; }
  bool include(int i) const { return _include.at<unsigned char>(i) == 1; }
  const Mat& mean_adjust() const { return _mean_adjust; }
  double mean_adjust(int i) const { return _mean_adjust.at<double>(i); }
  const Mat& range_scale() const { return _range_scale; }
  double range_scale(int i) const { return _range_scale.at<double>(i); }
  const Mat& projection() const { return _projection; }
  double projection(int i) const { return _projection.at<double>(i); }

  const Transform switch_include(const Mat &i) const
  {
    assert(i.cols <= output_dimension);
    return Transform(input_dimension, i.cols, i, _mean_adjust,
                     _range_scale, _projection);
  }
  const Transform switch_mean(const Mat &m) const
  {
    return Transform(input_dimension, output_dimension, _include, m,
                     _range_scale, _projection);
  }
  const Transform switch_range(const Mat &r) const
  {
    return Transform(input_dimension, output_dimension, _include, _mean_adjust,
                     r, _projection);
  }
  const Transform switch_projection(const Mat &p) const
  {
    assert(p.rows <= output_dimension);
    return Transform(input_dimension, p.rows, _include, _mean_adjust,
                     _range_scale, p);
  }
};

class Data {
public:
  const Mat features;
  const Mat labels;
  const Transform transform;

  Data(const Mat &f, const Mat &l, const Transform &t)
    : features(f), labels(l), transform(t)
  {
    assert(f.type() == CV_64F);
    assert(l.type() == CV_64F);
    assert(f.rows == l.rows); // All features have labels.
    assert(f.cols == t.output_dimension); // Project to correct dimensionality.
  }
  ~Data() {}

  const Mat feature(int i) const { return features.col(i); }
  double point(int i, int j) const { return features.at<double>(i, j); }
  double label_point(int i, int j) const { return labels.at<double>(i, j); }
  const Mat feature_data(int i) const { return features.row(i); }
  const Mat label_data(int i) const { return labels.row(i); }

  int num_features() const { return features.cols; }
  int num_labels() const { return labels.cols; }
  int num_data() const { return features.rows; }

  void describe(std::ostream &s, const std::string &msg) const;
  void describe_features(std::ostream &s) const;

  // Note: These two functions only transforms labels, so does not need to be
  // recorded in 'transform' as this does not affect the test set (except for
  // evaluating effectiveness).
  Data speedup_labels() const;
  Data probabilistic_labels() const;

  Data append(const Data &other) const;
  Data cut_label(int label) const;
  Data cut_feature(int feature) const;
  Data cut_bad_features() const;
  Data cut_profile_features() const;
  Data cut_empty_features() const;
  Data adjust_means() const;
  Data scale_ranges(bool use_stddev) const;
  Data apply_pca(double variance) const;
  Data apply_pca(int target_features) const;

private:
  Data pca_helper(PCA &pca) const;
  const Mat calculate_means(const Mat &features) const;
  const Mat calculate_ranges(const Mat &features) const;
};

const Data transform_labels(const Data &data, int knn_k, bool cut_x86,
                            bool cut_tilera, bool cut_gpu, bool speedup,
                            bool prob);
const Transform build_identity_transform(int dimensions);
const Transform load_transform(const char *fname);

#endif // _ML_DATA_HH
