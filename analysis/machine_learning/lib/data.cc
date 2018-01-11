#include "data.hh"
#include <iostream>

DataManager::DataManager(Mat32F &f, Mat32F &l) : _features(f), _labels(l),
                                                 _higher(false), _quiet(false)
{
  assert((f.rows() == l.rows())
         && "Features and labels must have the same number of rows.");
//  assert((l.min() > 0.0) && "Zero labels cause havoc!");
  for (int i = 0; i < l.cols(); i++) {
    _label_names.push_back(std::string("Unknown"));
  }
}

void DataManager::append(const DataManager &other)
{
  assert(num_features() == other.num_features() && "Data mismatch.");
  assert(num_labels() == other.num_labels() && "Data mismatch.");
  assert((transform().length() == 0) && "Can't append post-transformation");
  assert((other.transform().length() == 0) && "Can't append transformed data");
  _features = _features.concat_vertical(other.features());
  _labels = _labels.concat_vertical(other.labels());
}


int DataManager::num_data() const
{
  return _features.rows();
}

int DataManager::num_features() const
{
  return _features.cols();
}

int DataManager::num_labels() const
{
  return _labels.cols();
}

void DataManager::describe(std::ostream &s, const std::string &msg) const
{
  if (_quiet) return;

  s << "# " << msg << ": " << num_data() << " datapoints (" << num_features()
    << " features, " << num_labels() << " labels each)." << std::endl;
}

// Really this should be variadic, but there aren't many use-cases.
void DataManager::describe(std::ostream &s, const std::string &msg, int val) const
{
  if (_quiet) return;

  int len = msg.length() + 10 + 1;
  char *buf = new char[len];
  int written = snprintf(buf, len, msg.c_str(), val);
  assert((written < len-1) && "Invalid format string.");
  s << "# " << buf << ": " << num_data() << " datapoints (" << num_features()
    << " features, " << num_labels() << " labels each)." << std::endl;
  delete[] buf;
}

const Mat32F& DataManager::features() const
{
  return _features;
}

const Mat32F& DataManager::labels() const
{
  return _labels;
}

Col32D DataManager::classifications() const
{
  Col32D classified(num_data());
  for (int i = 0; i < num_data(); i++) {
    if (speedups())
      classified.at(i) = _labels.row(i).max_index();
    else
      classified.at(i) = _labels.row(i).min_index();
  }
  return classified;
}

Row32F DataManager::datapoint_features(int row) const
{ 
  return _features.row(row);
}

Row32F DataManager::datapoint_labels(int row) const
{
  return _labels.row(row);
}

Col32F DataManager::feature(int feature) const
{
  return _features.col(feature);
}

Col32F DataManager::label(int label) const
{
  return _labels.col(label);
}

const TransformManager& DataManager::transform() const
{
  return _transform;
}

TransformManager& DataManager::transform()
{
	return _transform;
}

const std::string& DataManager::label_name(int label) const
{
  assert(((label >= 0) && (label < num_labels())) && "Invalid index");
  return _label_names[label];
}

void DataManager::set_label_name(int label, const std::string &name)
{
  assert(((label >= 0) && (label < num_labels())) && "Invalid index");
  _label_names[label] = name;
}

DataManager load_data(const std::string &file, int num_labels)
{
  Mat32F data = load_csv32(file);

  // Split into "features" and "labels"
  std::pair<Mat32F, Mat32F> split = data.split(data.cols() - num_labels);
  Mat32F features = split.first;
  Mat32F labels = split.second;

  // DataManager doesn't like zero-valued labels, it messes up analysis.  So we
  // replace any 0's with the maximum (non-infinite) floating point value.  But
  // really it is better to not have the zero's there in the first place.
  bool warn = false;
  for (int i = 0; i < labels.rows(); i++) {
    for (int j = 0; j < labels.cols(); j++) {
      if (labels.at(i,j) == 0.0) {
        labels.at(i,j) = std::numeric_limits<float>::max();
        warn = true;
      }
    }
  }

  // TODO: Library code shouldn't use std::cout.
  if (warn)
    std::cout << "# WARNING: Zero-labels were set to FLOAT_MAX (" << file
              << ")." << std::endl;


  DataManager dm(features, labels);
  
  // TODO Actually load and set column names
  for (int i = 0; i < num_labels; i++) {
    std::ostringstream format;
    format << "arch" << i << "_suitability";
    dm.set_label_name(i, format.str());
  }
  
  return dm;
}

DataManager load_data(const std::vector<std::string> &files, int num_labels)
{
  std::vector<DataManager> managers;
  
  for (auto file : files) {
    managers.push_back(load_data(file, num_labels));
  }

  DataManager dm = managers.back();
  managers.pop_back();
  while (!managers.empty()) {
    dm.append(managers.back());
    managers.pop_back();
  }

  return dm;
}

std::ostream& operator<< (std::ostream &s, DataManager &dm)
{
  for (int i = 0; i < dm.num_data(); i++) {
    s << dm.datapoint_features(i) << " --> "
      << dm.datapoint_labels(i) << std::endl;
  }
  return s;
}
