// Usage:
//    ./train [OPTIONS]
//
//      --data $FILE    A program CSV file.  May be specified multiple times.
//      --arch $FILE    An arch' CSV file.  May be specified multiple times.
//
//      --arches $INT   The number of architectures involved. (DEFAULT: 3)
//
//      --perf-prediction Print performance prediction instead of architecture selection
//
//      --save-nn       Save trained NNs to file.
//      --no-save-nn    Or not. (DEFAULT)
//
//      --save-trans    Save the transform to file.
//      --no-save-trans Or not. (DEFAULT)
//
//      --cut-bad       Eliminate broken features. TODO tbr
//      --no-cut-bad    Or not. (DEFAULT)
//
//      --cut-prof      Cut profiling-dependent features.
//      --no-cut-prof   Or not. (DEFAULT)
//
//      --cut-empty     Cut 'empty' features. (DEFAULT)
//      --no-cut-empty  Or not.
//
//      --cut-features  Feature(s) to cut.
//      --cut-labels    Label(s) to cut. TODO not added to any transform manager
//
//      --speedup       Predict speedups. (DEFAULT)
//      --no-speedup    Or not.
//
//      --prob          Predict probabilities, compatible with above. TODO tbr
//      --no-prob       Or not. (DEFAULT)
//
//      --scale-means   Scale features to have zero-mean. (DEFAULT)
//      --no-scale-means  Or not.
//
//      --scale-ranges  Scale all features to have similar ranges. (DEFAULT)
//      --no-scale-ranges Or not.
//      --use-stddev    Use standard-deviation when scaling ranges.
//      --no-use-stddev Or not. (DEFAULT)
//
//      --knn $INT      Value to use for k-nearest-neighbours. (DEFAULT: 5) TODO tbr
//
//      --pca $INT      Apply $INT dimension PCA to benchmarks. (DEFAULT: 8)
//      --pca-arch $INT Apply $INT dimension PCA to architectures. (DEFAULT: 5)
//      --no-pca        Don't use PCA.
//
//      --nn            Use a neural network for learning. (DEFAULT)
//      --iter $INT     Number of iterations to train the NN. (DEFAULT: 1000)
//      --epsilon $NUM  NN error termination. (DEFAULT: 0.0000005)
//      --nn-backprop   Use the BACKPROP NN algorithm. (DEFAULT)
//      --nn-rprop      Use the RPROP NN algorithm.
//      --rate $NUM     NN training parameter. (DEFAULT: 0.1)
//      --momentum $NUM NN training parameter. (DEFAULT: 0.1)
//      --layer $LIST   NN intermediate layer shape. (DEFAULT: 6):
//
//      --svm           Use a support vector machine for learning.
//      --iter $INT     Number of iterations to train the SVM. (DEFAULT: 1000)
//      --epsilon $NUM  SVM error termination. (DEFAULT: 0.0000005)
//      --folds         N-fold search for optimal SVM params. (DEFAULT: 10)
//      --svm-c-svc     Create a C-SVC SVM model. (DEFAULT)
//      --svm-nu-svc    Create a NU-SVC SVM model.
//      --svm-linear    Use a linear kernel for SVM.
//      --svm-poly      Use a polynomial kernel for SVM.
//      --svm-rbf       Use a radial basis function kernel for SVM.
//      --svm-sigmoid   Use a sigmoid kernel for SVM. (DEFAULT)
//
//      --dtree         Use a decision tree for learning.
//      --depth $INT    Maximum depth of decision tree (DEFAULT: 50)
//      --min-samples $INT Min # training samples for classification category (DEFAULT: 10)
//      --max-cat $INT  Max number of classification categories (DEFAULT: 5)
//
//      --visualise     Output data for gnuplot post-processing.
//      --no-visualise  Or not. (DEFAULT)
//      --2d            Set visualisation to 2D. (DEFAULT)
//      --3d            Set visualisation to 3D.
//      --3d-speedup    Set visualisation to 3D, 3rd dimension is performance.
//      --verbose       Print extra stuff.
//      --no-verbose    Or not. (DEFAULT)
//
//      --no-sanity     Don't sanity check
//      --train $FILE   Training data file (don't mix with --data)
//      --test $FILE    Testing data file(s) (don't mix with --data)

#include "mat.hh"
#include "common.hh"
#include "pca.hh"
#include "predict.hh"
#include "eval.hh"
#include "ml_data.hh"
#include "nn.hh"
#include "utils.hh"
#include "timer.hh"
#include <iostream>
#include <sstream>

typedef enum { TWO_D, THREE_D, THREE_DS } visualisation_type;
typedef enum { NN, SVM, DTREE } learning_type;
static bool do_visualise = false;
static visualisation_type vtype = TWO_D;
static bool verbose = false;

static std::vector<std::string> data;/* List of CSV files to process. */
static std::vector<std::string> arch_data;/* List of CSV files to process. */
static std::string training_data; /* Training data file */
static std::vector<std::string> testing_data;  /* Testing data file */
static int num_arches = 3;        /* The number of arch' in the data. */
static bool perf_predict = false; /* Print performance predictions rather than architecture choices */
static bool cut_bad = false;       /* Cut "bad" features. TODO remove */ 
static bool cut_prof = false;     /* Cut profiling features. */
static bool cut_empty = true;     /* Cut empty features. */
static bool speedup = true;       /* Convert runtimes to speedups. */
static bool prob = false;         /* Convert labels to probabilities. */
static bool scale_means = true;   /* Scale features to have zero-mean. */
static bool scale_ranges = true;  /* Scale features to similar ranges. */
static bool use_stddev = false;   /* Use standard-deviation to scale ranges. */
static bool save_nns = false;     /* Save trained neural network to file. */
static bool save_trans = false;   /* Save feature transforms to file. */
static bool sanity = true;        /* Perform sanity check before actual testing. */
static int knn_k = 5;             /* Value of k for k-nearest-neighbours. */
static int pca = 8;               /* PCA dimensionality, -1 means no PCA. */
static int pca_arch = 5;          /* PCA dimensionality, -1 means no PCA. */
static learning_type model = NN;  /* Which machine learning model to use. */
static int nn_type = CvANN_MLP_TrainParams::BACKPROP;/* NN algorithm to use.*/
static int iter = 1000;           /* Maximum number of iterations to train. */
static double epsilon = 0.0000005;/* Error to train the NN to. */
static double rate = 0.1;         /* NN "rate". */
static double momentum = 0.1;     /* NN "momentum". */
static Col32D layers = {6};       /* NN layer structure. */
static int svm_folds = 10;        /* How many 'folds' train_auto should use. */
static int svm_type = CvSVM::C_SVC;/* SVM type. */
static int svm_kernel = CvSVM::SIGMOID;/* SVM kernel type. */
static int max_depth = 50;        /* Max decision tree depth. */
static int min_sample_count = 10; /* Min # samples per category in decision tree. */
static int max_categories = 5;    /* Max categories in decision tree. */
static std::set<int> cut_features;/* Manually specified features to cut */
static std::set<int> cut_labels;/* Manually specified labels to cut */

// Use PCA to reduce to 2-dimensions for the sake of visualisation.
static void visualise(DataManager &data)
{
  std::cout << std::setprecision(9) << std::fixed << std::endl;
  std::cout << std::endl << "# Visualisation data:" << std::endl;

  // Reduce data to 2 or 3 dimensions (depending on mode).
  data.apply_pca(vtype == THREE_D ? 3 : 2);

  for (int i = 0; i < data.num_data(); i++) {
    const Row32F features = data.datapoint_features(i);
    const Row32F labels = data.datapoint_labels(i);
    std::string p = std::string("Point ");
    double x = features.at(0);
    double y = features.at(1);
    int best_arch = speedup ? labels.max_index() : labels.min_index();
    const std::string &arch = data.label_name(best_arch);
    if (vtype == TWO_D) {
      std::cout << x << ", " << y << " # " << arch << " " <<p << i << std::endl;
    } else if (vtype == THREE_D) {
      double z = features.at(2);
      std::cout << x << ", " << y << ", " << z << " # " << arch << " "
                << p << i << std::endl;
    } else if (vtype == THREE_DS) {
      for (int j = 0; j < num_arches; j++) {
        std::cout << x << ", " << y << ", " << labels.at(j)
          << " # " << data.label_name(j) << " " << p << i << std::endl;
      }
    }
  }
}

void parse_args(int argc, char const* const* argv)
{
  int i = 0;
  while (i < argc) {
    if (strcmp(argv[i], "--data") == 0) {
      assert((i+1) < argc);
      data.push_back(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--arch") == 0) {
      assert((i+1) < argc);
      arch_data.push_back(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--arches") == 0) {
      assert((i+1) < argc);
      num_arches = parse_int(argv[i+1]);
      i++; // Skip the next argument.
		} else if (strcmp(argv[i], "--perf-prediction") == 0) {
			perf_predict = true;
    } else if (strcmp(argv[i], "--save-nn") == 0) {
      save_nns = true;
    } else if (strcmp(argv[i], "--no-save-nn") == 0) {
      save_nns = false;
    } else if (strcmp(argv[i], "--save-trans") == 0) {
      save_trans = true;
    } else if (strcmp(argv[i], "--no-save-trans") == 0) {
      save_trans = false;
    } else if (strcmp(argv[i], "--visualise") == 0) {
      do_visualise = true;
    } else if (strcmp(argv[i], "--no-visualise") == 0) {
      do_visualise = false;
    } else if (strcmp(argv[i], "--2d") == 0) {
      vtype = TWO_D;
    } else if (strcmp(argv[i], "--3d") == 0) {
      vtype = THREE_D;
    } else if (strcmp(argv[i], "--3d-speedup") == 0) {
      vtype = THREE_DS;
    } else if (strcmp(argv[i], "--verbose") == 0) { // TODO CUT?
      verbose = true;
    } else if (strcmp(argv[i], "--no-verbose") == 0) {
      verbose = false;
    } else if (strcmp(argv[i], "--cut-bad") == 0) {
      cut_bad = true;
    } else if (strcmp(argv[i], "--no-cut-bad") == 0) {
      cut_bad = false;
    } else if (strcmp(argv[i], "--cut-prof") == 0) { // TODO CUT?
      cut_prof = true;
    } else if (strcmp(argv[i], "--no-cut-prof") == 0) {
      cut_prof = false;
    } else if (strcmp(argv[i], "--cut-empty") == 0) { // TODO CUT?
      cut_empty = true;
    } else if (strcmp(argv[i], "--no-cut-empty") == 0) {
      cut_empty = false;
    } else if (strcmp(argv[i], "--speedup") == 0) {
      speedup = true;
    } else if (strcmp(argv[i], "--no-speedup") == 0) {
      speedup = false;
    } else if (strcmp(argv[i], "--prob") == 0) {
      prob = true;
    } else if (strcmp(argv[i], "--no-prob") == 0) {
      prob = false;
    } else if (strcmp(argv[i], "--scale-means") == 0) {
      scale_means = true;
    } else if (strcmp(argv[i], "--no-scale-means") == 0) {
      scale_means = false;
    } else if (strcmp(argv[i], "--scale-ranges") == 0) {
      scale_ranges = true;
    } else if (strcmp(argv[i], "--no-scale-ranges") == 0) {
      scale_ranges = false;
    } else if (strcmp(argv[i], "--use-stddev") == 0) { // TODO CUT?
      use_stddev = true;
    } else if (strcmp(argv[i], "--no-use-stddev") == 0) {
      use_stddev = false;
    } else if (strcmp(argv[i], "--knn") == 0) { // TODO CUT?
      assert((i+1) < argc);
      knn_k = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--pca") == 0) {
      assert((i+1) < argc);
      pca = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--pca-arch") == 0) {
      assert((i+1) < argc);
      pca_arch = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--no-pca") == 0) {
      pca = -1;
      pca_arch = -1;
    } else if (strcmp(argv[i], "--nn") == 0) {
      model = learning_type::NN;
    } else if (strcmp(argv[i], "--iter") == 0) {
      assert((i+1) < argc);
      iter = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--epsilon") == 0) {
      assert((i+1) < argc);
      epsilon = parse_float(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--nn-backprop") == 0) {
      nn_type = CvANN_MLP_TrainParams::BACKPROP;
    } else if (strcmp(argv[i], "--nn-rprop") == 0) {
      nn_type = CvANN_MLP_TrainParams::RPROP;
    } else if (strcmp(argv[i], "--rate") == 0) {
      assert((i+1) < argc);
      rate = parse_float(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--momentum") == 0) {
      assert((i+1) < argc);
      momentum = parse_float(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--layer") == 0) {
      assert((i+1) < argc);
      layers = parse_layers(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--svm") == 0) {
      model = learning_type::SVM;
    } else if (strcmp(argv[i], "--folds") == 0) {
      assert((i+1) < argc);
      svm_folds = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--svm-c-svc") == 0) {
      svm_type = CvSVM::C_SVC;
    } else if (strcmp(argv[i], "--svm-nu-svc") == 0) {
      svm_type = CvSVM::NU_SVC;
    } else if (strcmp(argv[i], "--svm-linear") == 0) {
      svm_kernel = CvSVM::LINEAR;
    } else if (strcmp(argv[i], "--svm-poly") == 0) {
      svm_kernel = CvSVM::POLY;
    } else if (strcmp(argv[i], "--svm-rbf") == 0) {
      svm_kernel = CvSVM::RBF;
    } else if (strcmp(argv[i], "--svm-sigmoid") == 0) {
      svm_kernel = CvSVM::SIGMOID;
    } else if (strcmp(argv[i], "--dtree") == 0) {
      model = learning_type::DTREE;
    } else if (strcmp(argv[i], "--depth") == 0) {
      max_depth = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--min-samples") == 0) {
      min_sample_count = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--max-cat") == 0) {
      max_categories = parse_int(argv[i+1]);
      i++; // Skip the next argument.
    } else if (strcmp(argv[i], "--cut-features") == 0) {
      assert(false && "\"--cut-features\" not yet implemented!");
      //cut_features = parse_comma_list(argv[i+1]);
      //i++;
    } else if (strcmp(argv[i], "--cut-labels") == 0) {
      assert(false && "\"--cut-labels\" not yet implemented!");
      //cut_labels = parse_comma_list(argv[i+1]);
      //i++; // Skip the next argument.
		} else if (strcmp(argv[i], "--no-sanity") == 0) {
			sanity = false;
		} else if (strcmp(argv[i], "--train") == 0) {
      assert((i+1) < argc);
      training_data = string(argv[i+1]);
      i++; // Skip the next argument.
		} else if (strcmp(argv[i], "--test") == 0) {
      assert((i+1) < argc);
			std::stringstream ss(argv[i+1]);
			std::string in;
			while(std::getline(ss, in, ','))
	      testing_data.push_back(in);
			i++; // Skip the next argument
    } else {
      assert(false && "Unrecognised command-line argument");
    }
    i++;
  }
}

// Explore PCA effectiveness for various variances.
static void explore_pca(const Mat32F &features)
{
  std::cout << "# Exploring PCA variances:" << std::endl;
  for (int i = 100; i > 85; i--) {
    double target = ((double)i)/((double)100);
    TransformProjectPtr projection = calculate_pca(features, target);
    std::cout << "# Retain " << target << " variance: reduced to "
              << projection->output_dimension() << " dimensions." << std::endl;
  }
}

static void evaluate_single(PredictionManager &pm,
                            const std::string &test_file)
{
  DataManager test_data = load_data(test_file, num_arches);
  test_data.quiet(true);
  standard_label_transform(test_data, knn_k, speedup, prob, cut_labels);

  EvaluationManager em(pm, num_arches);
	if(perf_predict)
		em.predict(test_data);
	else
	  em.choose(test_data);

  int slash = test_file.find_last_of('/');
  std::string name = test_file.substr(slash+1);
  evaluation_single(em, name, perf_predict);
}

static void evaluate(bool leaveoneout, EvaluationManager &em,
                     const TransformManager &training_transform,
                     const std::vector<std::string> &training_files,
                     const std::vector<std::string> &test_files)
{
  // Load the training and test data files.
  DataManager training_data = load_data(training_files, num_arches);
  DataManager test_data = load_data(test_files, num_arches);

  // Now, cleanup the loaded data a bit.  Firstly, apply the supplied set of
  // transformations to the training data (this same sequence will get picked
  // up by the prediction manager during training, and then applied to the test
  // data item by item).  Secondly, clean up the labels of both the training
  // and test data so that we can make sensible predictions.
  training_data.quiet(true);
  test_data.quiet(true);
  training_data.apply_sequence(training_transform);
  standard_label_transform(training_data, knn_k, speedup, prob, cut_labels);
  standard_label_transform(test_data, knn_k, speedup, prob, cut_labels);

	// Rob: labels are "unfiltered" whereas features have been scaled &
	// normalized.  We probably need to get them both on the same level.
	//
	// For Eurosys work, we could normalize by converting runtimes to speedups
	// over CPU.

  // Train the model!
  int iter = em.train(training_data);
	if(save_nns) {
		// Rob: save neural networks (assumes leave-one-out scenario).  If testing
		// on more files this isn't necessarily correct!
		std::string fname = test_files[0];
		fname = fname.substr(fname.rfind("/")+1);
		fname.resize(fname.rfind("."));
		fname += ".xml";
		em.predictor().save(fname);
	}

//  if (!leaveoneout) {
    em.predictor().describe(std::cout);
    std::cout << "# Trained for " << iter << " iterations." << std::endl;
//  }

  // Test the model!
	if(perf_predict) {
		// Gather performance predictions per-architecture
		em.predict(test_data);

		if (!leaveoneout) header_single();
		for(auto test_file : test_files) {
			evaluate_single(em.predictor(), test_file);
		}
	} else {
		// Choose architectures
	  em.choose(test_data);

	  if (!leaveoneout) header_single();
	  for (auto test_file : test_files) {
	    evaluate_single(em.predictor(), test_file);
    }
  }
}

#if 0
static std::vector<std::string> filter(const std::vector<std::string> &in,
                                       const std::string &pattern)
{
  std::vector<std::string> found;
  for (auto check : in) {
    if (check.find_first_of(pattern) != std::string::npos)
      found.push_back(check);
  }
  return found;
}

static void evalmulti(bool leaveoneout, EvaluationManager &em,
                     const TransformManager &training_transform,
                     const std::vector<std::string> &training_files,
                     const std::vector<std::string> &test_files)
{
  // Load the training and test data files.
  DataManager training_opteron = load_data(filter(training_files, "/opteron/"),
                                           num_arches);
  training_opteron.quiet(true);
  DataManager training_i7 = load_data(filter(training_files, "/i7/"),
                                           num_arches);
  training_i7.quiet(true);
  DataManager training_xeon = load_data(filter(training_files, "/xeon/"),
                                           num_arches);
  training_xeon.quiet(true);
  DataManager training_xeonphi = load_data(filter(training_files, "/xeonphi/"),
                                           num_arches);
  training_xeonphi.quiet(true);
  DataManager training_tesla = load_data(filter(training_files, "/tesla/"),
                                           num_arches);
  training_tesla.quiet(true);
  DataManager training_titan = load_data(filter(training_files, "/titan/"),
                                           num_arches);
  training_titan.quiet(true);
  DataManager training_radeon = load_data(filter(training_files, "/radeon/"),
                                           num_arches);
  training_radeon.quiet(true);
  DataManager test_data = load_data(test_files, num_arches);

  // Now, cleanup the loaded data a bit.  Firstly, apply the supplied set of
  // transformations to the training data (this same sequence will get picked
  // up by the prediction manager during training, and then applied to the test
  // data item by item).  Secondly, clean up the labels of both the training
  // and test data so that we can make sensible predictions.
  test_data.quiet(true);
  training_data.apply_sequence(training_transform);

  // Train the model!
  int iter = em.train(training_data);
  if (!leaveoneout) {
    em.predictor().describe(std::cout);
    std::cout << "# Trained for " << iter << " iterations." << std::endl;
  }

  // Test the model!
  em.choose(test_data);

  if (!leaveoneout) header_single();
  for (auto test_file : test_files) {
    evaluate_single(em.predictor(), test_file);
  }
}
#endif

static void process_single_system(const TransformManager &training_transform)
{
  // Build every machine learning container.  It's quite cheap, so build them
  // all and then pick the one that will be used.
  NeuralNetwork nn(iter, epsilon, nn_type, rate, momentum, layers);
  SupportVectorMachineC svm(iter, epsilon, svm_folds, svm_type, svm_kernel);
  DecisionTreeC dtree(max_depth, min_sample_count, min_sample_count);
  PredictionManager *ml = nullptr;
  if (model == learning_type::NN) {
    ml = &nn;
  } else if (model == learning_type::SVM) {
    ml = &svm;
  } else if (model == learning_type::DTREE) {
    ml = &dtree;
  }
  assert((ml != nullptr) && "No learning model was set");

  // Evaluate the model's effectiveness.  First run on the training data (a bit
  // unorthodox, but it lets us spot major breakage), second run on the test
  // data.
	if(sanity) {
	  title(std::cout, "SANITY CHECK PREDICTOR");
	  EvaluationManager sanity_eval(*ml, num_arches);
	  evaluate(false, sanity_eval, training_transform, data, data);
	  evaluation_report(sanity_eval);
	  spacer(std::cout);
	}

  // Perform leave-one-out cross validation on the input files (as long as we
  // have at least 2 files).
	if(training_data != "" && testing_data.size() > 0) {
		title(std::cout, "EVALUATE PREDICTOR");
		header_single();
	  EvaluationManager test_eval(*ml, num_arches);
    std::vector<std::string> training = { training_data };
    evaluate(true, test_eval, training_transform, training, testing_data);
    evaluation_report(test_eval);
    spacer(std::cout);
  } else if (data.size() > 1) {
    title(std::cout, "EVALUATE PREDICTOR");
    header_single();
    EvaluationManager test_eval(*ml, num_arches);
    for (auto leaveoneout : data) {
      std::vector<std::string> training;
      std::vector<std::string> test = { leaveoneout };
      for (auto d : data) {
        if (d != leaveoneout) training.push_back(d);
      }
      evaluate(true, test_eval, training_transform, training, test);
    }
    evaluation_report(test_eval);
    spacer(std::cout);
	}
}

// TODO: This and above function.
static void process_multi_system(const TransformManager &training_transform)
{
  assert((unsigned)num_arches == arch_data.size());
  RegressionManagerPtr ml = nullptr;
  if (model == learning_type::NN) {
    ml = std::make_shared<NeuralNetwork>(iter, epsilon, nn_type, rate,
                                         momentum, layers);
  } else if (model == learning_type::SVM) {
    assert(false && "Classifiers (SVMs) not compatible with multi-system");
  }
  assert((ml != nullptr) && "No learning model was set");

  title(std::cout, "LOADING ARCHES");
  // The Python pre-processing tool hard-codes in one junk label.
  DataManager arches = load_data(arch_data, 1);
  arches.describe(std::cout, "Loaded all arch data");
  standard_feature_transform(arches, cut_empty, scale_means, scale_ranges, cut_features);
  spacer(std::cout);

  title(std::cout, "ARCHITECTURE PCA EXPERIMENTS");
  explore_pca(arches.features());
  spacer(std::cout);

  title(std::cout, "TRANSFORM ARCH DATA");
  if (pca_arch > 0) {
    // Reduce to an N-dimensional space using principle component analysis.
    arches.apply_pca(pca);
    arches.describe(std::cout, "Applied %d-dim PCA", pca);
    assert((arches.num_features() <= pca) && "PCA did not get applied.");
  }
  spacer(std::cout);

  RegressionMulti multi_ml(ml);
  for (int i = 0; i < arches.num_data(); i++) {
    multi_ml.add(arches.datapoint_features(i));
  }

  // Evaluate the model's effectiveness.  First run on the training data (a bit
  // unorthodox, but it lets us spot major breakage), second run on the test
  // data.
  title(std::cout, "SANITY CHECK PREDICTOR");
  EvaluationManager sanity_eval(multi_ml, num_arches);
  evaluate(false, sanity_eval, training_transform, data, data);
  evaluation_report(sanity_eval);
  spacer(std::cout);

  // Perform leave-one-out cross validation on the input files (as long as we
  // have at least 2 files).
  if (data.size() > 1) {
    title(std::cout, "EVALUATE PREDICTOR");
    header_single();
    EvaluationManager test_eval(multi_ml, num_arches);
    for (auto leaveoneout : data) {
      std::vector<std::string> training;
      std::vector<std::string> test = { leaveoneout };
      for (auto d : data) {
        if (d != leaveoneout) training.push_back(d);
      }
      evaluate(true, test_eval, training_transform, training, test);
    }
    evaluation_report(test_eval);
    spacer(std::cout);
  }
}

int main(int argc, char **argv)
{
  parse_args(argc-1, &(argv[1]));
	assert((data.size() >= 1) || (training_data != "" && testing_data.size() > 0));
  //assert(data.size() >= 1 && "No data was provided.");

  set_default_precision(std::cout);

  // Load the training and test data from files.
	if(data.size() == 0) {
		data.push_back(training_data);
		for(unsigned i = 0; i < testing_data.size(); i++)
			data.push_back(testing_data[i]);
	}
  title(std::cout, "LOADING");
  DataManager all_data = load_data(data, num_arches);
  all_data.describe(std::cout, "Loaded all data");

  // Now, before we do any feature tranformations we transform the labels, so
  // that they are always in the form that we want.
  standard_label_transform(all_data, knn_k, speedup, prob, cut_labels);
  spacer(std::cout);

  // Now we clean up the features of the training data so that it is possible
  // to make sensible statistical statements about them.  Do not clean up the
  // test data, as it should automatically have the transformation sequence
  // built for the training data applied at prediction time.  Having a separate
  // cleaning sequence makes no sense for test data as in the standard use case
  // (one prediction at a time) no clean-up is possible.
  title(std::cout, "DATA CLEANING");
  standard_feature_transform(all_data, cut_empty, scale_means, scale_ranges,
    cut_features);
  spacer(std::cout);

  // If we are running the visualisation code then quit once that is done, the
  // visualiser doesn't care about performing machine learning, and it will
  // apply 2 or 3-dimensional PCA (hampering future stages).
  if (do_visualise) { visualise(all_data); return EXIT_SUCCESS; }

  // Transform the training data.  These might be more significant changes than
  // the previous cleaning, previously we should have only removed noise or
  // explicitly cut data, now we might remove some signal.
  title(std::cout, "TRANSFORM DATA");
  if (pca > 0) {
	  title(std::cout, "BENCHMARK PCA EXPERIMENTS");
	  explore_pca(all_data.features());
	  spacer(std::cout);

    // Reduce to an N-dimensional space using principle component analysis.
    all_data.apply_pca(pca);
    all_data.describe(std::cout, "Applied %d-dim PCA", pca);
    assert((all_data.num_features() <= pca) && "PCA did not get applied.");
  }
  spacer(std::cout);

	// Rob: save after all data sanitization & PCA analysis
	if(save_trans) {
		std::string trans_fname("trans.xml");
		all_data.transform().save(trans_fname);
	}

  // TODO: Get its own flag?
  if (arch_data.size() == 0)
    process_single_system(all_data.transform());
  else
    process_multi_system(all_data.transform());

  return 0;
}
