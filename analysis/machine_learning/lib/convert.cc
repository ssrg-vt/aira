#include "data.hh"
#include "pca.hh"
#include <limits>

void DataManager::scale_labels()
{
	// TODO determine from labels
	float max = 215.0f;
	float min = 0.0f;

	for (int i = 0; i < num_data(); i++) {
		for (int j = 0; j < _labels.cols(); j++) {
			_labels.at(i, j) = (_labels.at(i, j) - min) / (max - min);
		}
	}
}

static void LabelToSpeedup(Row32F &label)
{
  float baseline_speedup = label.at(0);

  for (int i = 0; i  < label.cols(); i++) {
    float other_speedup = label.at(i);
    label.at(i) = baseline_speedup / other_speedup;
  }
}

void DataManager::convert_labels_to_speedups()
{
  for (int i = 0; i < num_data(); i++) {
    Row32F label = datapoint_labels(i);
    LabelToSpeedup(label);
  }
  _higher = true;
}

static void LabelToProbability(Row32F &label)
{
  float sum = label.sum();
  for (int i = 0; i  < label.cols(); i++) {
    label.at(i) = label.at(i) / sum;
  }
}

void DataManager::convert_labels_to_probabilities()
{
  for (int i = 0; i < num_data(); i++) {
    Row32F label = datapoint_labels(i);
    LabelToProbability(label);
  }
}

void DataManager::apply_sequence(const TransformManager tm)
{
  _features = tm.apply(_features);
  _transform.add(tm);
}

void DataManager::apply_empty_cut()
{
  Row32F ranges = _features.ranges();
  std::set<int> to_cut;
  for (int i = 0; i < ranges.cols(); i++) {
    if (ranges.at(i) < std::numeric_limits<float>::epsilon()) {
      to_cut.insert(i);
    }
  }

  if (!to_cut.empty()) {
    TransformProjectPtr cut = BuildTransformCut(num_features(), to_cut);
    _features = cut->apply(_features);
    _transform.add(cut);
  }
}

void DataManager::apply_scale_means()
{
  // Covert data to doubles before calculating means as the intermediate values
  // for calculating the mean (i.e. the sum) can be enormous for large
  // datasets.
  Mat64F features64 = _features.convert64();
  Row64F means = features64.means();
  Row64F negative_means(means.cols());
  for (int i = 0; i < means.cols(); i++) {
    negative_means.at(i) = -means.at(i);
  }
  Row32F negative_means32 = negative_means.convert32().row(0);

  TransformShiftPtr shift = std::make_shared<TransformShift>(negative_means32);
  _features = shift->apply(_features);
  _transform.add(shift);
}

void DataManager::apply_scale_ranges()
{
  Row32F ranges = _features.ranges();
  Row32F inverse_ranges(ranges.cols());
  for (int i = 0; i < ranges.cols(); i++) {
    if (ranges.at(i) == 0.0) {
      // Mathematically this is nonsense, but zero-size ranges are both common
      // and uninteresting, so get rid of them by scaling to zero.
      inverse_ranges.at(i) = 0.0;
    } else {
      inverse_ranges.at(i) = 1.0 / ranges.at(i);
    }
  }

  TransformScalePtr scale = std::make_shared<TransformScale>(inverse_ranges);
  _features = scale->apply(_features);
  _transform.add(scale);
}

void DataManager::apply_pca(int dimension)
{
  TransformProjectPtr projection = calculate_pca(_features, dimension);
  _features = projection->apply(_features);
  _transform.add(projection);
}

void DataManager::apply_feature_cut(std::set<int>& to_cut)
{
  TransformProjectPtr cut = BuildTransformCut(num_features(), to_cut);
  _features = cut->apply(_features);
  _transform.add(cut);
}

void DataManager::apply_label_cut(std::set<int>& to_cut)
{
  TransformProjectPtr cut = BuildTransformCut(num_labels(), to_cut);
  _labels = cut->apply(_labels);
	//TODO add to transform?
}

