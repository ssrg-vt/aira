#include <iomanip>
#include "ml_data.hh"
#include "knn.hh"
#include "utils.hh"

#include "include/mat.hh"

const Transform build_identity_transform(int dimensions)
{
  const Mat include = Mat::ones(1, dimensions, CV_8U);
  const Mat mean_adjust = Mat::zeros(1, dimensions, CV_64F);
  const Mat range_scale = Mat::ones(1, dimensions, CV_64F);
  const Mat projection = Mat::eye(dimensions, dimensions, CV_64F);
  return Transform(dimensions, dimensions, include, mean_adjust, range_scale,
                   projection);
}

const Data transform_labels(const Data &data, int knn_k, bool cut_x86,
                            bool cut_tilera, bool cut_gpu, bool speedup,
                            bool prob)
{
  // ---------------------------------------------------------------------------
  // Fill in the blanks for the labels, i.e. when a given data-point does
  // not have a runtime for a particular architecture (but is marked as being
  // compatible with that architecture) use the K nearest neigbours to estimate
  // a runtime.  Note that this should be a no-op on static input data.

  // First we apply some pre-cleaning on data, so that all features are treated
  // equally (otherwise features with large ranges dominate).  Cut the
  // compatability features however, we'll restore them from the original data
  // below.
  const int compatability_start = data.num_features()-data.num_labels();
  assert(data.num_labels() == 3 && "TODO: Need to write a loop here.");
  const Data clean_data = data.cut_bad_features() \
                              .cut_feature(compatability_start) \
                              .cut_feature(compatability_start+1) \
                              .cut_feature(compatability_start+2) \
                              .adjust_means() \
                              .scale_ranges(false);

  const Data *strip = &clean_data;
  const Data strip_data = clean_data.cut_feature(0).cut_feature(1) \
                                    .cut_feature(2).cut_feature(3) \
                                    .cut_feature(4).cut_feature(5) \
                                    .cut_feature(6).cut_feature(7) \
                                    .cut_feature(8).cut_feature(9) \
                                    .cut_feature(10).cut_feature(11) \
                                    .cut_feature(12).cut_feature(13) \
                                    .cut_feature(14).cut_feature(15) \
                                    .cut_feature(16).cut_feature(17) \
                                    .cut_feature(18).cut_feature(19) \
                                    .cut_feature(20).cut_feature(21) \
                                    .cut_feature(23).cut_feature(24) \
                                    .cut_feature(25).cut_feature(26) \
                                    .cut_feature(29);
  if (clean_data.num_features() == 33 ) { // TODO: Don't hard-code this number.
    strip = &strip_data;
  }

  // We need to preserve the compatibility entries from the original data
  // though, so take real-feature columns from strip, and compatibility
  // columns from data.
  //                                   X                       Y
  // -------- DATA_FEATURES -------- | ------ DATA_COMPAT ------
  const int X = data.num_features()-data.num_labels();
  const int Y  = data.num_features();
  Mat merge_features;
  hconcat(strip->features, data.features.colRange(X, Y), merge_features);
  const Data knn_in_data = Data(merge_features, strip->labels,
                                build_identity_transform(merge_features.cols));

  // Now we are ready to actually do kNN!
  Timer t; t.start();
  const KNN knn(knn_in_data);
  const Data knn_pre = knn.fill_gaps(knn_in_data, knn_k);
  t.stop(); std::cout << "# Time to fill blanks with kNN: " << t << std::endl;

  // We do not want to actually modify the features within this function, so
  // build a new Data using the old features/transform, but the new labels.
  const Data knn_data = Data(data.features, knn_pre.labels, data.transform);
  // ---------------------------------------------------------------------------

  // From here, the structure of this function is like transform_features(...).
  const Data *in_progress = &knn_data;

  const Data cut_x86_data = in_progress->cut_label(X86);
  if (cut_x86) {
    in_progress = &cut_x86_data;
    in_progress->describe(std::cout, std::string("Removed ") + arch_str(X86));
  }

  const Data cut_tilera_data = in_progress->cut_label(TILERA);
  if (cut_tilera) {
    in_progress = &cut_tilera_data;
    in_progress->describe(std::cout,
                          std::string("Removed ") + arch_str(TILERA));
  }

  const Data cut_gpu_data = in_progress->cut_label(GPU);
  if (cut_gpu) {
    in_progress = &cut_gpu_data;
    in_progress->describe(std::cout, std::string("Removed ") + arch_str(GPU));
  }

  // Convert performance data to speedups.  Note that kNN must be run before
  // this (if it is going to be run at all).
  const Data speedup_data = in_progress->speedup_labels();
  if (speedup) {
    in_progress = &speedup_data;
    in_progress->describe(std::cout, "Convert labels to speedups");
  }

  // Convert performance data to "probabilities":
  const Data prob_data = in_progress->probabilistic_labels();
  if (prob) {
    in_progress = &prob_data;
    in_progress->describe(std::cout, "Convert labels to prob'");
  }

  return *in_progress;
}

const Transform load_transform(const char *fname)
{
  int input_dimension;
  int output_dimension;
  Mat include;
  Mat mean_adjust;
  Mat range_scale;
  Mat projection;

  FileStorage fs(fname, FileStorage::READ);
  fs["input_dimension"] >> input_dimension;
  fs["output_dimension"] >> output_dimension;
  fs["include"] >> include;
  fs["mean_adjust"] >> mean_adjust;
  fs["range_scale"] >> range_scale;
  fs["projection"] >> projection;
  return Transform(input_dimension, output_dimension, include, mean_adjust,
                   range_scale, projection);
}

void Transform::save(const char *fname) const
{
  FileStorage fs(fname, FileStorage::WRITE);
  fs << "input_dimension" << input_dimension;
  fs << "output_dimension" << output_dimension;
  fs << "include" << _include;
  fs << "mean_adjust" << _mean_adjust;
  fs << "range_scale" << _range_scale;
  fs << "projection" << _projection;
}

void Transform::project(const Mat &in, Mat &out) const
{
  //assert((in.rows == 1) && (in.cols == input_dimension));

  // Apply the include vector
  Mat in_progress(1, input_dimension, CV_64F);
  int include_count = 0;
  for (int i = 0; i < input_dimension; i++) {
    if (_include.at<unsigned char>(i) == 1) {
      in_progress.at<double>(include_count++) = in.at<double>(i);
    }
  }

  // Scale means and ranges.
  Mat in_progress2 = in_progress.colRange(0, include_count);
  Mat in_progress3 = in_progress2 - _mean_adjust;
  Mat in_progress4 = in_progress3 / _range_scale;

  // Apply PCA projection
  Mat in_progress5 = _projection * in_progress4.t();
  out = in_progress5.t(); // TODO can we manage zero transposes?
}

void Data::describe(std::ostream &s, const std::string &msg) const
{
  s << "# " << msg << ": " << num_data() << " datapoints (" << num_features()
    << " features, " << num_labels() << " labels each)." << std::endl;
}

void Data::describe_features(std::ostream &s) const
{
  const Mat means = calculate_means(features);
  const Mat ranges = calculate_ranges(features);

  s << std::setprecision(3) << std::scientific;

  s << "#\t\tMean\t\tRange\t\tWarnings" << std::endl;
  for (int i = 0; i < num_features(); i++) {
    double m = means.at<double>(i);
    double r = ranges.at<double>(i);
    s << "# Feature " << i << ":\t" << m << ",\t" << r;
    if (r < 0.01) s << "\t!!!";
    s << std::endl;
  }

  set_default_precision(s);
}

Data Data::speedup_labels() const
{
  const Mat new_labels(num_data(), num_labels(), CV_64F);

  for (int i = 0; i < num_data(); i++) {
    const Mat &label = label_data(i);

    Mat speedups = Mat(1, num_labels(), CV_64F);
    for (int j = 0; j < num_labels(); j++) {
      speedups.at<double>(j) = calc_speedup(label.at<double>(0),
                                            label.at<double>(j));
    }
    speedups.copyTo(new_labels.row(i));
  }

  return Data(features, new_labels, transform);
}

Data Data::probabilistic_labels() const
{
  const Mat new_labels(num_data(), num_labels(), CV_64F);

  for (int i = 0; i < num_data(); i++) {
    const Mat &label = label_data(i);

    Mat prob = Mat(1, num_labels(), CV_64F);
    for (int j = 0; j < num_labels(); j++) {
      prob.at<double>(j) = label.at<double>(j) / sum(label)(0);
    }
    prob.copyTo(new_labels.row(i));
  }

  return Data(features, new_labels, transform);
}

Data Data::append(const Data &other) const
{
  // TODO: Project other.features?
  Mat combined_features, combined_labels;
  vconcat(features, other.features, combined_features);
  vconcat(labels, other.labels, combined_labels);
  return Data(combined_features, combined_labels, transform);
}

Data Data::cut_label(int label) const
{
  Mat new_labels(num_data(), num_labels(), CV_64F);

  for (int i = 0; i < num_labels(); i++) {
    if (i != label) {
      labels.col(i).copyTo(new_labels.col(i));
    } else {
      Mat z = Mat::zeros(num_data(), 1, CV_64F);
      z.copyTo(new_labels.col(i));
    }
  }

  return Data(features, new_labels, transform);
}

Data Data::cut_feature(int cut_index) const
{
  assert(cut_index < transform.include().cols);

  // Create the matrices where we will store the new data.
  Mat _features(num_data(), num_features()-1, CV_64F);
  Mat _include = transform.include().clone();
  Mat _mean_adjust(1, num_features()-1, CV_64F);
  Mat _range_scale(1, num_features()-1, CV_64F);

  // Mask out this feature in the include vector.
  assert(_include.at<unsigned char>(cut_index) == 1);
  _include.at<unsigned char>(cut_index) = 0;
  assert(approx(sum(_include)(0), num_features()-1));

  int in_col = 0, out_col = 0;
  for (int i = 0; i < _include.cols; i++) {
    if (i == cut_index) in_col++;
    if (_include.at<unsigned char>(i) == 1) {
      feature(in_col).copyTo(_features.col(out_col));
      _mean_adjust.at<double>(out_col) = transform.mean_adjust(in_col);
      _range_scale.at<double>(out_col) = transform.range_scale(in_col);
      in_col++;
      out_col++;
    }
  }
  assert(out_col == num_features()-1);

  // Cutting items out of the projection matrix is probably the wrong thing to
  // do, so just produce a new identity matrix.
  assert((transform.projection().rows == transform.projection().cols)
         && "Empty features must be cut before any projection matrices are "
            "produced (i.e. before PCA is run)");
  const Mat _projection = Mat::eye(num_features()-1, num_features()-1, CV_64F);

  return Data(_features, labels, Transform(transform.input_dimension,
                                           transform.output_dimension-1,
                                           _include, _mean_adjust,
                                           _range_scale, _projection));
}

Data Data::cut_bad_features() const
{
  if (transform.include().cols == 35) {
    return cut_feature(22).cut_feature(27).cut_feature(28);
  } else if (transform.include().cols == 39) {
    return cut_feature(22).cut_feature(27).cut_feature(28);
  } else {
    assert(false && "Unrecognised feature count.");
  }
}

Data Data::cut_profile_features() const
{
  if (transform.include().cols == 35) {
    Data t1 = cut_feature(1).cut_feature(3).cut_feature(5).cut_feature(7);
    Data t2 = t1.cut_feature(9).cut_feature(11).cut_feature(13);
    Data t3 = t2.cut_feature(15).cut_feature(17).cut_feature(19);
    Data t4 = t3.cut_feature(21).cut_feature(24).cut_feature(26);
    if (transform.include(28) == 1) return t4.cut_feature(28);
    return t4;
  } else if (transform.include().cols == 39) {
    Data t1 = cut_feature(1).cut_feature(3).cut_feature(5).cut_feature(7);
    Data t2 = t1.cut_feature(9).cut_feature(11).cut_feature(13);
    Data t3 = t2.cut_feature(15).cut_feature(17).cut_feature(19);
    Data t4 = t3.cut_feature(21).cut_feature(24).cut_feature(26);
    if (transform.include(28) == 1) return t4.cut_feature(28);
    return t4;
  } else {
    assert(false && "Unrecognised feature count.");
  }
}

// Cut all columns where every value is roughly identical (concretely: cut
// every column where max-min is < 0.01).
Data Data::cut_empty_features() const
{
  const Mat ranges = calculate_ranges(features);

  // Count the number of "interesting" columns.
  int col = 0;
  for (int i = 0; i < transform.include().cols; i++) {
    if (approx(transform.include(i), 1.0)) {
      if (ranges.at<double>(col) <= 0.01) {
        return cut_feature(i).cut_empty_features();
      }
      col++;
    }
  }

  return *this;
}

Data Data::adjust_means() const
{
  const Mat means = calculate_means(features);

  Mat adjusted_features(num_data(), num_features(), CV_64F);
  for (int i = 0; i < num_data(); i++) {
    for (int j = 0; j < num_features(); j++) {
      double old_val = point(i,j);
      double m = means.at<double>(j);
      adjusted_features.at<double>(i,j) = old_val - m;
    }
  }

  return Data(adjusted_features, labels, transform.switch_mean(means));
}

Data Data::scale_ranges(bool use_stddev) const
{
  Mat ranges(1,num_features(), CV_64F);

  if (use_stddev) {
    ranges = calculate_ranges(features);
  } else {
    for (int j = 0; j < num_features(); j++) {
      Scalar stddev;
      meanStdDev(feature(j), noArray(), stddev);
      ranges.at<double>(j) = stddev(0);
    }
  }

  Mat adjusted_features(num_data(), num_features(), CV_64F);
  for (int i = 0; i < num_data(); i++) {
    for (int j = 0; j < num_features(); j++) {
      double old_val = point(i,j);
      double r = ranges.at<double>(j);
      assert(((approx(old_val, 0.0) == (r == 0.0)) || (r > 0.0))
             && "Either both the orignal and r are 0, or else r is > 0.");
      double scale = (r == 0.0) ? 0.0 : (old_val / r);
      assert(std::isfinite(scale) && "Screwy maths");
      adjusted_features.at<double>(i,j) = scale;
    }
  }

  return Data(adjusted_features, labels, transform.switch_range(ranges));
}

Data Data::apply_pca(double variance) const
{
  PCA pca(features, noArray(), CV_PCA_DATA_AS_ROW, variance);
  return pca_helper(pca);
}

Data Data::apply_pca(int target_features) const
{
  PCA pca(features, noArray(), CV_PCA_DATA_AS_ROW, target_features);
  return pca_helper(pca);
}

Data Data::pca_helper(PCA &pca) const
{
  const Mat& projection = pca.eigenvectors;
  const Mat reduced_features_t = projection * features.t();
  const Mat reduced_features = reduced_features_t.t();
  assert(reduced_features.rows == num_data());
  return Data(reduced_features, labels,
              transform.switch_projection(projection));
}

const Mat Data::calculate_means(const Mat &features) const
{
  Mat means(1, features.cols, CV_64F);
  for (int i = 0; i < features.cols; i++) {
    Scalar m = mean(features.col(i));
    means.at<double>(i) = m(0);
  }
  return means;
}

const Mat Data::calculate_ranges(const Mat &features) const
{
  Mat ranges(1, features.cols, CV_64F);
  for (int i = 0; i < features.cols; i++) {
    double minVal, maxVal;
    Point minLoc, maxLoc; // Not used.
    minMaxLoc(features.col(i), &minVal, &maxVal, &minLoc, &maxLoc);
    ranges.at<double>(i) = (maxVal - minVal);
  }
  return ranges;
}
