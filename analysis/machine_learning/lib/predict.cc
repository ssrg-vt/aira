#include "predict.hh"
#include <cmath>
#include <cassert>
#include <iostream>

PredictionManager::~PredictionManager() {}

Row32F PredictionManager::predict(const Row32F &features) const
{
	assert(false && "Attempted to print performance predictions using an unsupported ML model!");
}

int RegressionManager::choose(const Row32F &features) const
{
  Row32F prediction = this->predict(features);
  return higher ? prediction.max_index() : prediction.min_index(); 
}

void RegressionMulti::add(const Row32F &extra)
{
  extras.push_back(extra);
}

// TODO: De-uglify
int RegressionMulti::train(const DataManager &data)
{
  Row32F features0 = data.datapoint_features(0);
  Mat32F new_features = features0.concat_horizontal(extras[0]);
  Mat32F new_labels(1,1);
  new_labels.at(0,0) = data.datapoint_labels(0).at(0);
  for (unsigned j = 1; j < extras.size(); j++) {
    new_features = new_features.concat_vertical(features0.concat_horizontal(extras[j]));
    Row32F label = { data.datapoint_labels(0).at(j) };
    new_labels = new_labels.concat_vertical(label);
  }

  for (int i = 1; i < data.num_data(); i++) {
    Row32F features = data.datapoint_features(i);
    for (unsigned j = 0; j < extras.size(); j++) {
#if 1
      // Bob
      if (j == 0) continue;
      if (j == 6) continue;
      if (j == 7) continue;
#endif

#if 0
      // Hulk
      if (j == 1) continue;
      if (j == 4) continue;
#endif

#if 0
      // Whitewhale
      if (j == 2) continue;
      if (j == 3) continue;
      if (j == 5) continue;
#endif
      new_features = new_features.concat_vertical(features.concat_horizontal(extras[j]));
      Row32F label = { data.datapoint_labels(i).at(j) };
      new_labels = new_labels.concat_vertical(label);
    }
  }

  // Replace the recorded transformation sequence (if any) with the sequence
  // used to clean up this training data.
  transform.clear();
  transform.add(data.transform());
  higher = data.speedups();
  
  DataManager new_data(new_features, new_labels);
  return manager->train(new_data);
}

void RegressionMulti::load(const std::string &file)
{
  // TODO
}

void RegressionMulti::describe(std::ostream &s) const
{
  // TODO
}

// TODO: verify
Row32F RegressionMulti::predict(const Row32F &features) const
{
  assert((features.cols() == input_dimension()) && "Invalid feature dimension");
  Row32F transformed_features = transform.apply(features);

  Row32F response(extras.size());
  for (unsigned i = 0; i < extras.size(); i++) {
    Row32F new_features = transformed_features.concat_horizontal(extras[i]).row(0);
    response.at(i) = manager->predict(new_features).at(0);
  }

#if 1
  // Bob
  Row32F r(3);
  r.at(0) = response.at(0);
  r.at(1) = response.at(6);
  r.at(2) = response.at(7);
#endif

#if 0
  // Hulk
  Row32F r(2);
  r.at(0) = response.at(1);
  r.at(1) = response.at(4);
#endif

#if 0
  // Whitewhale
  Row32F r(3);
  r.at(0) = response.at(2);
  r.at(1) = response.at(3);
  r.at(2) = response.at(5);
#endif

  return r;
}

void RegressionMulti::save(const std::string &file) const
{
  // TODO
}

int NeuralNetwork::train(const DataManager &data)
{
  assert((data.num_data() > 0) && "Need training data to train network.");

  if (nn) delete nn;  // Delete the old neural network, if there is one.

  // The user only specifies the size of middle layers, the size of the first
  // and last layer are defined by the data.
  Col32D layers(middle_layers.rows() + 2);
  layers.at(0) = data.num_features();
  layers.at(layers.rows()-1) = data.num_labels();
  for (int i = 0; i < middle_layers.rows(); i++)
    layers.at(i+1) = middle_layers.at(i);

  // Termination parameters for training.
  CvTermCriteria term = cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS,
                                       max_iter, epsilon);

  // Training parameters for neural network.
  CvANN_MLP_TrainParams params(term, type, rate, momentum);

  // Build an NN with the requested shape and train it.
  CvANN_MLP *tmp = new CvANN_MLP(layers.mat());
  int iter = tmp->train(data.features().mat(), data.labels().mat(),
                        cv::Mat(), cv::Mat(), // Don't provide weightings.
                        params,
                        CvANN_MLP::NO_INPUT_SCALE | CvANN_MLP::NO_OUTPUT_SCALE);
  nn = tmp; // Save in the const* (the NN is now unmodifiable).

  // Replace the recorded transformation sequence (if any) with the sequence
  // used to clean up this training data.
  transform.clear();
  transform.add(data.transform());
  higher = data.speedups();

  return iter;
}

void NeuralNetwork::load(const std::string &file)
{
	if(nn) delete nn;

	cv::FileStorage fs(file.c_str(), cv::FileStorage::READ);
	CvANN_MLP *tmp = new CvANN_MLP();
	tmp->load(file.c_str(), "nn");
	nn = tmp;
}

void NeuralNetwork::describe(std::ostream &s) const
{
  assert((nn != nullptr) && "No neural network built yet.");
  // The cast below is only required due to a missing const annotation in
  // OpenCV, the call is const though.
  Row32D layers(((CvANN_MLP *) nn)->get_layer_sizes());
  s << "# NN"
    << ", I: " << max_iter
    << ", E: " << epsilon
    << ", R: " << rate
    << ", M: " << momentum
    << ", L: " << layers
    << std::endl;
}

Row32F NeuralNetwork::predict(const Row32F &features) const
{
  assert((nn != nullptr) && "No neural network built yet.");
  // TODO -1 is okay
  //assert((features.cols() == input_dimension()) && "Invalid feature dimension");
  Row32F transformed_features = transform.apply(features);
  cv::Mat out;
  nn->predict(transformed_features.mat(), out);
  return Row32F(out);
}

void NeuralNetwork::save(const std::string &file) const
{
	assert(nn != nullptr && "No trained neural network");
	assert(file.rfind(".xml") != file.npos
		&& "Neural network file does not end in \".xml\"!");

	cv::FileStorage fs(file, cv::FileStorage::WRITE);
	nn->write(*fs, "nn");
}

int SupportVectorMachineC::train(const DataManager &data)
{
  assert((data.num_data() > 0) && "Need training data to SVM.");

  if (svm) delete svm;  // Delete the old SVM, if there is one.

  // TODO: Make configurable.  No effect on train_auto?
  double degree = 1.0;
  double gamma = 1.0;
  double coef0 = 0.0;
  double Cvalue = 1.0;
  double nu = 0.0;
  double p = 0.0;
  
  // Termination parameters for training.
  CvTermCriteria term = cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS,
                                       max_iter, epsilon);

  CvSVMParams params(type, kernel, degree, gamma, coef0, Cvalue, nu, p,
                     nullptr, term);

  CvSVM *tmp = new CvSVM();
  cv::Mat features = data.features().mat();
  cv::Mat labels = data.classifications().convert32().mat();
  if (folds == 1) {
    tmp->train(features, labels,
               cv::Mat(), cv::Mat(), // Don't provide weightings.
               params);
  } else {
    tmp->train_auto(features, labels,
                    cv::Mat(), cv::Mat(), // Don't provide weightings.
                    params, folds);
  }
  svm = tmp; // Save in the const* (the SVM is now unmodifiable).

  // Replace the recorded transformation sequence (if any) with the sequence
  // used to clean up this training data.
  // TODO: This is now duplicated, refactor.
  transform.clear();
  transform.add(data.transform());
  higher = data.speedups();

  return 1; // No concept of iterations in an SVM.
}

void SupportVectorMachineC::load(const std::string &file)
{
  // TODO
}

void SupportVectorMachineC::describe(std::ostream &s) const
{
  assert((svm != nullptr) && "No SVM built yet.");
  CvSVMParams params = svm->get_params();
  s << "# SVM"
    << ": degree = " << params.degree
    << ", gamma = " << params.gamma
    << ", coef0 = " << params.coef0
    << ", C = " << params.C << "," << std::endl
    << "#    "
    << "  nu = " << params.nu
    << ", p = " << params.p << std::endl; 
}

int SupportVectorMachineC::choose(const Row32F &features) const
{
  assert((svm != nullptr) && "No SVM built yet.");
  assert((features.cols() == input_dimension()) && "Invalid feature dimension");
  Row32F transformed_features = transform.apply(features);
  float prediction = svm->predict(transformed_features.mat());
  return lround(prediction);
}

void SupportVectorMachineC::save(const std::string &file) const
{
  // TODO
}

int DecisionTreeC::train(const DataManager &data)
{
  assert((data.num_data() > 0) && "Need training data to train decision tree.");

  if (dtree) delete dtree;  // Delete the old Decision Tree, if there is one.

  // TODO: Make configurable.
  float regression_accuracy = 0.0000005; //Termination criteria for regression trees (need?)
  bool use_surrogates = true; //No clue...
  int cv_folds = 5; //After training, prune the tree with K-fold cross validation
  bool use_1se_rule = false; //Something about making the pruning harsher, disabled for now
  bool truncate_pruned_tree = true; //Remove (rather than disable) pruned pieces
  const float* priors = nullptr; //Prior distribution values

  // Training parameters
  CvDTreeParams params(max_depth, min_sample_count, regression_accuracy,
        use_surrogates, max_categories, cv_folds, use_1se_rule,
        truncate_pruned_tree, priors);

  CvDTree *tmp = new CvDTree();
  cv::Mat features = data.features().mat();
  cv::Mat labels = data.classifications().convert32().mat();
  tmp->train(features, CV_ROW_SAMPLE, labels, cv::Mat(), cv::Mat(), cv::Mat(),
             cv::Mat(), params);

  dtree = tmp; // Save in the const* (the SVM is now unmodifiable).

  // Replace the recorded transformation sequence (if any) with the sequence
  // used to clean up this training data.
  // TODO: This is now duplicated, refactor.
  transform.clear();
  transform.add(data.transform());
  higher = data.speedups();

  return 1; // No concept of iterations in a decision tree
}

void DecisionTreeC::load(const std::string &file)
{
  // TODO not implemented
}

int DecisionTreeC::getHeight()
{
	assert(dtree != nullptr && "Cannot get height if decision tree has not been trained");

	int height = 0;
	getHeightInternal(dtree->get_root(), height);
	return height;
}

void DecisionTreeC::getHeightInternal(const CvDTreeNode* curNode, int &curHeight)
{
	//Post-order traversal
	if(curNode->left != nullptr)
		getHeightInternal(curNode->left, curHeight);
	if(curNode->right != nullptr)
		getHeightInternal(curNode->right, curHeight);

	//Check this node
	if(curNode->depth > curHeight)
		curHeight = curNode->depth;
}

void DecisionTreeC::describe(std::ostream &s) const
{
  // TODO
}

int DecisionTreeC::choose(const Row32F &features) const
{
  assert((dtree != nullptr) && "No decision tree built yet.");
  assert((features.cols() == input_dimension()) && "Invalid feature dimension");
  Row32F transformed_features = transform.apply(features);
  float prediction = dtree->predict(transformed_features.mat())->value;
  return lround(prediction);
}

void DecisionTreeC::save(const std::string &file) const
{
  // TODO not implemented
}

