#include "nn.hh"
#include "ml_data.hh"
#include "utils.hh"

void SVM_Classifier::build(const Data &data)
{
  // Define what type of SVM we want.
  CvSVMParams params;
  params.svm_type = CvSVM::C_SVC;
  //params.svm_type = CvSVM::NU_SVC;
  //params.kernel_type = CvSVM::RBF;
  params.kernel_type = CvSVM::SIGMOID;

  /*
  // Optimal for Static C_SVC + RBF
  params.degree = 0.0;
  params.gamma = 0.00015;
  params.coef0 = 0.0;
  params.C = 0.1;
  params.nu = 0.0;
  params.p = 0.0;
  */

  // Optimal for Static C_SVC + SIGMOID
  params.degree = 0.0;
  params.gamma = 0.00001;
  params.coef0 = 19.6;
  params.C = 0.1;
  params.nu = 0.0;
  params.p = 0.0;

  /*
  // Optimal for Static NU_SVC + SIGMOID
  params.degree = 0.0;
  params.gamma = 0.506;
  params.coef0 = 0.1;
  params.C = 0.0;
  params.nu = 0.01;
  params.p = 0.0;
  */

  /*
  // Optimal for Static NU_SVC + RBF
  params.degree = 0.0;
  params.gamma = 0.034;
  params.coef0 = 0.0;
  params.C = 0.0;
  params.nu = 0.09;
  params.p = 0.0;
  */

  /*
  CvTermCriteria term;
  term.type = CV_TERMCRIT_ITER | CV_TERMCRIT_EPS;
  term.max_iter = 1000;
  term.epsilon = FLT_EPSILON;
  params.term_crit = term;
  */

  CvSVM *svm_tmp = new CvSVM();

  // Convert our regression information into classification.
  Mat classification(data.num_data(), 1, CV_32F);
  for (int i = 0; i < data.num_data(); i++) {
    classification.at<float>(i) = find_best_arch(data.label_data(i), true);
  }

  // Convert features to 32-bit floats.
  Mat f(data.num_data(), data.num_features(), CV_32F);
  data.features.convertTo(f, CV_32F);

  const int folds = 10; // TODO make this configurable
  Mat empty1, empty2;
  svm_tmp->train_auto(f, classification, empty1, empty2, params, folds);
  //svm_tmp->train(f, classification, empty1, empty2, params);

  CvSVMParams opt = svm_tmp->get_params();
  std::cout << "# Optimal parameters"
            << ": degree = " << opt.degree
            << ", gamma = " << opt.gamma
            << ", coef0 = " << opt.coef0
            << ", C = " << opt.C << "," << std::endl
            << "#                   "
            << "  nu = " << opt.nu
            << ", p = " << opt.p << std::endl;

  svm = svm_tmp;  // Assign to the const*, the SVM can no longer change.
}

void SVM_Classifier::predict(const Mat &in, Mat &out) const
{
  // Project the input to the correct scale, dimensionality etc.
  Mat projected;
  t.project(in, projected);

  // Convert the label to 32-bit floats.
  Mat tmp(projected.rows, projected.cols, CV_32F);
  projected.convertTo(tmp, CV_32F);

  // Convert the classification back to a "regression".
  float val = svm->predict(tmp);
  out.at<double>(0) = ((val < 0.5) ? 1.0 : 0.0);
  out.at<double>(1) = (((val >= 0.5) && (val < 1.5)) ? 1.0 : 0.0);
  out.at<double>(2) = ((val >= 1.5) ? 1.0 : 0.0);
}

void SVM_Classifier::save(const char *fname) const
{
  // TODO
  svm->save(fname);
}

void SVM_Classifier::load(const char *fname)
{
  assert(svm == NULL);
  CvSVM *new_svm = new CvSVM();
  new_svm->load(fname);
  svm = new_svm;
}
