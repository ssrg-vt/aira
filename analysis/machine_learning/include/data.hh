#ifndef _DATA_HH
#define _DATA_HH

#include "mat.hh"
#include "transform.hh"
#include <vector>
#include <string>

class DataManager {
private:
  Mat32F _features;
  Mat32F _labels;
  TransformManager _transform;
  std::vector<std::string> _label_names;
  bool _higher;
  bool _quiet;
  
public:
  DataManager(Mat32F &f, Mat32F &l);

  // Add more data (implementation in data.cc).
  void append(const DataManager &other);

  // Statistics about the data (implementations in data.cc).
  int num_data() const;
  int num_features() const;
  int num_labels() const;
  bool speedups() const { return _higher; }
  void quiet(bool val) { _quiet = val; }
  void describe(std::ostream &s, const std::string &msg) const;
  void describe(std::ostream &s, const std::string &msg, int val) const;

  // Data accessors (implementations in data.cc).
  const Mat32F& features() const;
  const Mat32F& labels() const;
  Col32D classifications() const;
  // TODO: The below four functions aren't really const. Fix it!
  Row32F datapoint_features(int row) const;
  Row32F datapoint_labels(int row) const;
  Col32F feature(int feature) const;
  Col32F label(int label) const;
  const TransformManager& transform() const;
	TransformManager& transform();

  // Name accessors (implementations in data.cc).
  const std::string& label_name(int label) const;
  void set_label_name(int label, const std::string &name);

  // Conversion functions (implementations in convert.cc).
	void scale_labels();
  void convert_labels_to_speedups();
  void convert_labels_to_probabilities();
  void apply_sequence(const TransformManager tm);
  void apply_empty_cut();
  void apply_scale_means();
  void apply_scale_ranges();
  void apply_pca(int dimension);
	void apply_feature_cut(std::set<int>& to_cut);
	void apply_label_cut(std::set<int>& to_cut);
};

DataManager load_data(const std::string &file, int num_labels);
DataManager load_data(const std::vector<std::string> &files, int num_labels);
std::ostream& operator<< (std::ostream &s, DataManager &dm);

#endif // _DATA_HH
