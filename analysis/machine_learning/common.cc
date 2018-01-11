#include "common.hh"
#include <iostream>
#include <string>

static double percent(double amount, double total)
{
  return (amount/total)*100;
}

static std::string format_ratio(int amount, int total)
{
  std::ostringstream format;
  format << amount << "/" << total << " (" << percent(amount,total) << "%)";
  return format.str();
}

static std::string justify(const std::string msg, unsigned minlen)
{
  std::string justified = msg;
  while (justified.length() < minlen)
    justified += " ";
  return justified;
}

void standard_label_transform(DataManager &data, int knn_k,
                              bool speedup, bool prob,
                              std::set<int>& cut_labels)
{
  // TODO: knn

	bool scaleRanges = true; // TODO make command-line arg
	if (scaleRanges) {
		data.scale_labels();
	}

  // Convert performance data to relative speed-ups.
  if (speedup) {
    data.convert_labels_to_speedups();
    data.describe(std::cout, "Convert labels to speedups");
  }

  // Convert performance data to "probabilities".
  if (prob) {
    data.convert_labels_to_probabilities();
    data.describe(std::cout, "Convert labels to prob");
  }

  if (cut_labels.size() > 0) {
    data.apply_label_cut(cut_labels);
    data.describe(std::cout, "Cut labels"); //TODO list cut labels
  }
}

void standard_feature_transform(DataManager &data, bool cut_empty,
                                bool scale_means, bool scale_ranges,
                                std::set<int>& cut_features)
{
#if 0 // TODO
  // Cut some known to be bad features manually.
  const Data clean_data = in_progress->cut_bad_features();
  if (cut_bad) {
    in_progress = &clean_data;
    in_progress->describe(std::cout, "Cut bad features");
  }
#endif

#if 0 // TODO
  // Optionally, cut profile-dependent features.
  const Data prof_data = in_progress->cut_profile_features();
  if (cut_prof) {
    in_progress = &prof_data;
    in_progress->describe(std::cout, "Cut profile-dependent features");
  }
#endif

  // Cut all features that add nothing.
  if (cut_empty) {
    data.apply_empty_cut();
    assert((data.num_features() > 0)
           && "cut empty removed ALL features, data is junk");
    data.describe(std::cout, "Cut empty features");
  }

  // Scale all features to have zero-mean.
  if (scale_means) {
    data.apply_scale_means();
    data.describe(std::cout, "Scale means");
  }

  // Scale all features to have similar ranges.
  // TODO: Can also try scaling by stddev rather than range.
  if (scale_ranges) {
    data.apply_scale_ranges();
    data.describe(std::cout, "Scale ranges");
  }

  if (cut_features.size() > 0) {
    data.apply_feature_cut(cut_features);
    data.describe(std::cout, "Cut features"); //TODO list cut features
  }
}

void evaluation_report(const EvaluationManager &em)
{
  std::cout << "# Average training time: " << em.training_times() << std::endl;
  std::cout << "# Average prediction time: " << em.prediction_times()
            << std::endl << std::endl;

  const Mat32D &confusion = em.confusion_matrix();
  std::cout << confusion << std::endl << std::endl;

  std::cout << "#   Correct distibution: " << confusion.transpose().sums()
            << std::endl;
  std::cout << "# Predicted distibution: " << confusion.sums() << std::endl;

  int correct = em.correct_predictions();
  int incorrect = em.incorrect_predictions();
  int total = correct+incorrect;
  std::cout << "# Correctly classified " << format_ratio(correct, total)
            << std::endl;

  std::cout << "# Speed-up: " << em.average_predicted() << " (oracle: "
            << em.average_actual() << ")" << std::endl;
}

void header_single()
{
  std::cout << "# --- " << justify("BENCHMARK", 20) << "ACCURACY,\t\tA --> P"
            << "\t\tMAX%" << std::endl;
}

void evaluation_single(const EvaluationManager &em, const std::string &name, 
											 const bool perf_predict)
{
	if(perf_predict) {
		double err = em.average_prediction_error();
		int total = em.num_predictions();
		std::cout << "# --- " << justify(name, 20) << err << "\t\t" << total;
	} else {
	  int correct = em.correct_predictions();
	  int incorrect = em.incorrect_predictions();
	  int total = correct+incorrect;
	  std::cout << "# --- " << justify(name, 20) << format_ratio(correct, total);

	  int predicted_best = em.confusion_matrix().sums().max_index();
	  int actual_best = em.confusion_matrix().transpose().sums().max_index();
	  if (predicted_best == actual_best)
	    std::cout << ",\t\t" << actual_best << " === " << predicted_best;
	  else
	    std::cout << ",\t\t" << actual_best << " --> " << predicted_best;

	  std::cout << "\t\t" << percent(em.average_predicted(), em.average_actual());
	  std::cout << "\t\t" << em.average_predicted();
		std::cout << "\t\t" << em.average_actual();
	}
	std::cout << std::endl;
}
