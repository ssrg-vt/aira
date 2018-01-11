#include "eval.hh"
#include <iostream>

EvaluationManager::EvaluationManager(PredictionManager &_pm, int num_arches)
  : pm(_pm), predictions(0), confusion(num_arches, num_arches),
    sum_actual(0.0), sum_predicted(0.0), prediction_error(0.0)
{
}

int EvaluationManager::train(const DataManager &dm)
{
  training_timer.start();
  int iter = pm.train(dm);
  training_timer.stop();
  return iter;
}

static double to_speedup(const Row32F &labels, int idx)
{
  double total = 1.0 / labels.at(0);
  return labels.at(idx) * total;
}

void EvaluationManager::choose(const Row32F &features, const Row32F &labels,
                               bool higher)
{
  prediction_timer.start();
  int predicted_best = pm.choose(features);
  prediction_timer.stop();

if (dynamic_cast<RegressionMulti*>(&pm) == nullptr) {
  int actual_best = higher ? labels.max_index() : labels.min_index();

  predictions++;
  confusion.at(actual_best, predicted_best) += 1;
  sum_actual += to_speedup(labels, actual_best);
  sum_predicted += to_speedup(labels, predicted_best);
} else {

#if 1
  // Bob
  Row32F l(3);
  l.at(0) = labels.at(0);
  l.at(1) = labels.at(6);
  l.at(2) = labels.at(7);
#endif

#if 0
  // Hulk
  Row32F l(2);
  l.at(0) = labels.at(1);
  l.at(1) = labels.at(4);
#endif

#if 0
  // Whitewhale
  Row32F l(3);
  l.at(0) = labels.at(2);
  l.at(1) = labels.at(3);
  l.at(2) = labels.at(5);
#endif

  int actual_best = higher ? l.max_index() : l.min_index();

  predictions++;
  confusion.at(actual_best, predicted_best) += 1;
  sum_actual += to_speedup(l, actual_best);
  sum_predicted += to_speedup(l, predicted_best);
}
}

void EvaluationManager::choose(const DataManager &dm)
{
  Mat32D confusion_old = confusion_matrix();

  for (int i = 0; i < dm.num_data(); i++) {
    choose(dm.datapoint_features(i), dm.datapoint_labels(i), dm.speedups());
  }

  Mat32D diff = confusion_matrix() - confusion_old;
}

void EvaluationManager::predict(const Row32F &features, const Row32F &labels)
{
  prediction_timer.start();
  Row32F prediction = pm.predict(features);
  prediction_timer.stop();

	predictions++;
	for(int idx = 0; idx < prediction.cols(); idx++) {
		prediction_error += fabs((prediction.at(idx) - labels.at(idx)) / labels.at(idx));
		std::cout << labels.at(idx) << " " << prediction.at(idx) << std::endl;
	}
}

void EvaluationManager::predict(const DataManager &dm)
{
	for (int i = 0; i < dm.num_data(); i++) {
    predict(dm.datapoint_features(i), dm.datapoint_labels(i));
  }
}

int EvaluationManager::correct_predictions() const
{
  int sum = 0;
  for (int i = 0; i < confusion.rows(); i++) {
    sum += confusion.at(i,i);
  }
  return sum;
}

int EvaluationManager::incorrect_predictions() const
{
  return confusion.sum() - correct_predictions();
}

double EvaluationManager::average_actual() const
{
  return sum_actual / predictions;
}

double EvaluationManager::average_predicted() const
{
  return sum_predicted / predictions;
}

int EvaluationManager::num_predictions() const
{
	return predictions;
}

double EvaluationManager::average_prediction_error() const
{
	return (prediction_error / predictions) * 100;
}
