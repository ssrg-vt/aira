#include "nn.hh"

void NN::build(const Data &data, int max_iter, double epsilon, double rate,
               double momentum, const Mat &layers)
{
  Mat complete_layers = layers;
  if (layers.at<int>(0) == -1)
    complete_layers.at<int>(0) = data.num_features();

  assert (complete_layers.at<int>(0) == data.num_features());
  assert (complete_layers.at<int>(layers.rows-1) == data.num_labels());

  std::cout << "# NN"
            << ", I: " << max_iter
            << ", E: " << epsilon
            << ", R: " << rate
            << ", M: " << momentum
            << ", L: " << complete_layers
            << std::endl;

  // Training termination parameters
  CvTermCriteria term;
  term.type = CV_TERMCRIT_ITER | CV_TERMCRIT_EPS;
  term.max_iter = max_iter;
  term.epsilon = epsilon;

  // Training neural network parameters
  CvANN_MLP_TrainParams params(term, CvANN_MLP_TrainParams::BACKPROP, rate,
                               momentum);

  // Build an NN with the requested shape and train it.
  CvANN_MLP *tmp = new CvANN_MLP(complete_layers);
  int iter = tmp->train(data.features, data.labels, Mat(), Mat(), params,
                        CvANN_MLP::NO_INPUT_SCALE | CvANN_MLP::NO_OUTPUT_SCALE);
  nn = tmp; // Save in the const* (the NN is now unmodifiable).

  std::cout << "# Trained for " << iter << " iterations." << std::endl;
}

void NN::predict(const Mat &in, Mat &out) const
{
  Mat projected;
  t.project(in, projected);
  nn->predict(projected, out);
}

void NN::load(const char *fname)
{
  assert(nn == NULL);
  CvANN_MLP *new_nn = new CvANN_MLP();
  new_nn->load(fname);
  nn = new_nn;
}

void NN::save(const char *fname) const
{
  nn->save(fname);
}
