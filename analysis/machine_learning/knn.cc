#include "knn.hh"

KNN::KNN(const Data &data)
{
  for (int i = 0; i < data.num_labels(); i++) {
    build(data, i);
  }
}

KNN::~KNN()
{
  std::vector<KNearest*>::iterator it;
  for (it = knns.begin(); it != knns.end(); ++it) {
    delete *it;
  }
}

void KNN::build(const Data &data, int label_index)
{
  Mat relevant_features(data.num_data(), data.num_features(), CV_32F);
  Mat relevant_labels(data.num_data(), 1, CV_32F);

  // The KNN code *only* works with single-precision floats.
  Mat converted_features;
  data.features.convertTo(converted_features, CV_32F);

  // Copy all rows with non-zero runtime.
  int count = 0;
  for (int i = 0; i < data.num_data(); i++) {
    double label = data.labels.at<double>(i,label_index);
    if (label > 0.1) {
      const Mat in_row = converted_features.row(i);
      in_row.copyTo(relevant_features.row(count));
      relevant_labels.at<float>(count) = (float)label;
      count++;
    }
  }

  if (count > 0) {
    KNearest *knn = new KNearest(relevant_features.rowRange(0,count),
                                 relevant_labels.rowRange(0,count),
                                 Mat(), true);
    knns.push_back(knn);
  } else {
    knns.push_back(NULL);
  }
}

Data KNN::fill_gaps(const Data &data, int k) const
{
  Mat knn_labels = Mat::zeros(data.num_data(), data.num_labels(), CV_64F);
  int filled = 0;

  for (int j = 0; j < data.num_labels(); j++) {
    KNearest *knn = knns[j];
    if (knn == NULL) continue; // We were not able to build kNN for this arch.
    for (int i = 0; i < data.num_data(); i++) {
      double label = data.label_point(i, j);
      double compat = data.point(i, data.num_features()-data.num_labels()+j);
      if ((label == 0.0) && approx(compat, 1.0)) {
        filled++;
        Mat features = data.feature_data(i);
        features.convertTo(features, CV_32F);
        float knn_label = knn->find_nearest(features, k);
        knn_labels.at<double>(i,j) = (double)knn_label;
      } else {
        knn_labels.at<double>(i,j) = label;
      }
    }
  }

  std::cout << "# kNN: Extrapolated " << filled << " new points, with k = "
    << k << "." << std::endl;

  return Data(data.features, knn_labels, data.transform);
}
