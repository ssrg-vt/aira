#ifndef _EVAL_HH
#define _EVAL_HH

#include "data.hh"
#include "predict.hh"
#include "timer.hh"

class EvaluationManager {
private:
  PredictionManager &pm;
  int predictions;
  Mat32D confusion;
  double sum_actual; // Sum of the best labels
  double sum_predicted; // Sum of the predicted labels
	double prediction_error;
  Timer training_timer;
  Timer prediction_timer;

public:
  EvaluationManager(PredictionManager &_pm, int num_arches);

  int train(const DataManager &dm);

  void choose(const Row32F &features, const Row32F &labels, bool higher);
  void choose(const DataManager &dm);

	void predict(const Row32F &features, const Row32F &label);
	void predict(const DataManager &dm);

  PredictionManager& predictor() const { return pm; }
  const Timer& training_times() const { return training_timer; }
  const Timer& prediction_times() const { return prediction_timer; }
  const Mat32D& confusion_matrix() const { return confusion; }

  int correct_predictions() const;
  int incorrect_predictions() const;
  double average_actual() const;
  double average_predicted() const;
	int num_predictions() const;
	double average_prediction_error() const;
};

#endif // _EVAL_HH
