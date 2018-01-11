#include <cstdio>
#include <mpfr.h>
#include "utils.hh"

static void evaluate(Mat32D &confusion, Mat32F &abs_error,
                     std::vector< std::vector<double> > &speedups,
                     const Row32F &predicted, const Row32F &real, bool speedup)
{
  int real_arch = speedup ? real.max_index() : real.min_index();
  int predicted_arch = speedup ? predicted.max_index() : predicted.min_index();
  abs_error = abs_error + (real-predicted).abs();

  // Pop this run into the 'confusion' matrix (real_arch == predicted_arch
  // actually indicates no confusion at all).
  confusion.at(real_arch, predicted_arch) += 1;

  // Record the speed-ups over arch-0: per-arch and then for the predicted arch
  // and the "correct" arch (i.e. an oracle prediction).
  for (int i = 0; i < NUM_ARCHES; i++) {
    speedups[i].push_back(calc_speedup(real.at(i), real.at(0),
                                       !speedup /* potential flip */));
  }
  speedups[NUM_ARCHES].push_back(calc_speedup(real.at(predicted_arch),
                                              real.at(0),
                                              !speedup /* potential flip */));
  speedups[NUM_ARCHES+1].push_back(calc_speedup(real.at(real_arch),
                                                real.at(0),
                                                !speedup /* potential flip */));
}

void evaluate_neural_network(const RegressionManager &nn,
                             const DataManager &test, bool speedup)
{
  // Initialise where we'll store statistical data.
  Mat32D confusion(NUM_ARCHES, NUM_ARCHES);
  Mat32F abs_error(1, NUM_ARCHES);
  std::vector< std::vector<double> > speedups(NUM_ARCHES+2);
  Timer timer;

  // Run the neural network over the test data.
  for (int i = 0; i < test.num_data(); i++) {
    timer.start();
    Row32F o = nn.predict(test.datapoint_features(i));
    timer.stop();
    evaluate(confusion, abs_error, speedups, o, test.datapoint_labels(i), speedup);
  }
  std::cout << "# Average time per data-item: " << timer << std::endl
            << std::endl;
  Mat32F mean_abs_error = abs_error / test.num_data();

  // Find and output the 'confusion' matrix.
  int correct = 0, wrong = 0;
  print_arches(std::cout, "\t  ", "\t<--- Predicted arches");
  for (int i = 0; i < NUM_ARCHES; i++) {
    std::cout << "# " << arch_str((arch)i) << "\t: ";
    for (int j = 0; j < NUM_ARCHES; j++) {
      int count = confusion.at(i,j);
      std::cout << count << "\t";
      if (i == j) { correct += count; } else { wrong += count; }
    }
    std::cout << std::endl;
  }

  // Use the confusion matrix data to summarise correct/incorrect
  // classifications.
  std::cout << std::endl << "# Correct classification: " << correct
            << ", Wrong: " << wrong << " ("
            << ((double)correct)/(correct+wrong)*100
            << "% correctly classified)" << std::endl;

  // Print the mean absolute error per-arch.
  std::cout << std::endl;
  print_arches(std::cout, "\t\t\t", "");
  std::cout << "# Mean absolute error: ";
  for (int i = 0; i < NUM_ARCHES; i++) {
    std::cout << "\t" << (mean_abs_error.at(0, i)*100.0) << "%";
  }
  std::cout << std::endl;

  // Print the speed-ups (over x86) per hard-coded arch and this NN.
  std::cout << std::endl;
  print_arches(std::cout, "\t\t\t", "NN\tOracle");
  std::cout << "# Speed-up over " << arch_str((arch)0) << ":\t";
  for (int i = 0; i < NUM_ARCHES+2; i++) {
    std::cout << geomean(speedups[i], true) << "\t";
  }
  std::cout << std::endl << "#           Average :\t";
  for (int i = 0; i < NUM_ARCHES+2; i++) {
    std::cout << average(speedups[i], true) << "\t";
  }
  std::cout << std::endl << "#      Full-average :\t";
  for (int i = 0; i < NUM_ARCHES+2; i++) {
    std::cout << average(speedups[i], false) << "\t";
  }
  std::cout << std::endl;
}

arch find_best_arch(const Mat &label, bool speedup)
{
  assert(label.rows == 1);
  assert(label.cols == NUM_ARCHES);
  double minVal, maxVal; // Not used.
  Point minLoc, maxLoc;
  minMaxLoc(label, &minVal, &maxVal, &minLoc, &maxLoc);
  if (speedup) {
    return (arch)maxLoc.x;
  } else {
    return (arch)minLoc.x;
  }
}

const char *arch_str(arch a)
{
  const char *best_arch = NULL;
  switch (a) {
    case X86: best_arch = "x86 "; break;
    case GPU: best_arch = "GPU "; break;
    case TILERA: best_arch = "Tile"; break;
    default: assert(false);
  }
  return best_arch;
}


// Handy printing functions.
void set_default_precision(std::ostream &s)
{
  s << std::setprecision(3) << std::fixed;
}

void title(std::ostream &s, const std::string &msg)
{
  int printed = 0;
  if (msg.size() > 0) {
    s << std::endl << "### " << msg << " ";
    printed = msg.size() + 5;
  }
  for (int i = printed; i < 79; i++) s << "#";
  s << std::endl;
}

void spacer(std::ostream &s)
{
  title(s, "");
}

void print_arches(std::ostream &s, const char *prestr, const char *poststr)
{
  s << "#" << prestr;
  for (int i = 0; i < NUM_ARCHES; i++) {
    s << arch_str((arch)i) << "\t";
  }
  s << poststr << std::endl;
}

int parse_int(const char *str)
{
  assert((strnlen(str, 100) < 12) && "Integer strings must be <12 characters.");
  int num;
  int count = sscanf(str, "%11d", &num);
  assert(count == 1 && "Integer required");
  return num;
}

double parse_float(const char *str)
{
  assert((strnlen(str, 100) < 100) && "Float strings must be <100 characters.");
  double num;
  int count = sscanf(str, "%99lf", &num);
  assert(count == 1 && "Floating point number required");
  return num;
}

Col32D parse_layers(const char *str)
{
  int i;
  std::vector<int> v;
  std::stringstream ss(str);

  while (ss >> i) {
    v.push_back(i);
    if (ss.peek() == ',') ss.ignore();
  }

  if (v.size() == 1) {
    return Col32D({v[0]});
  } else if (v.size() == 2) {
    return Col32D({v[0], v[1]});
  } else if (v.size() == 3) {
    return Col32D({v[0], v[1], v[2]});
  } else if (v.size() == 4) {
    return Col32D({v[0], v[1], v[2], v[3]});
  } else if (v.size() == 5) {
    return Col32D({v[0], v[1], v[2], v[3], v[4]});
  } else if (v.size() == 6) {
    return Col32D({v[0], v[1], v[2], v[3], v[4], v[5]});
  }

  assert(false && "Must have between 2 and 6 layers.");
}

std::set<int> parse_comma_list(const char *str)
{
	int i;
	std::set<int> v;
	std::stringstream ss(str);

	while (ss >> i) {
		v.insert(i);
		if (ss.peek() == ',') ss.ignore();
	}

	return v;
}

double average(const std::vector<double> &list, bool skip_blanks) {
  double count = 0;
  unsigned skipped = 0;
  std::vector<double>::const_iterator it;
  for (it = list.begin(); it != list.end(); ++it) {
    double val = *it;
    if ((val == 0.0) && skip_blanks) {
      skipped++;
    } else {
      count += val;
    }
  }
  if (list.size() == skipped) return 0.0;  // Avoid a div-by-0.
  return count / (list.size() - skipped);
}

// Geometric mean: product(list) ^ (1/list.size())
//
// We are multipling so many tiny values together (easily thousands <0.001)
// that we have to use arbitrary precision arithmetic, even 128-bit floats
// didn't work (they eventually rounded off to zero).  Note that the final
// answer will fit in a double without issue, it is the intermediate values
// that are tiny (e.g., the final value of count).
//
// Note that although 'skip_blanks' is optional, setting it to false will
// result in an answer of '0' is there are any blanks.
double geomean(const std::vector<double> &list, bool skip_blanks) {

  // Number of bits of precision and rounding mode.
  mpfr_prec_t precision = std::max(list.size()*2, 128UL);
  mpfr_rnd_t round = GMP_RNDN;

  // Set the count variable to 1, this will store the rolling product.
  mpfr_t count;
  mpfr_init2(count, precision);
  mpfr_set_ui(count, 1UL, round);
  unsigned skipped = 0;

  // Calculate: product(list)
  std::vector<double>::const_iterator it;
  for (it = list.begin(); it != list.end(); ++it) {
    double val = *it;
    if ((val == 0.0) && skip_blanks) {
      skipped++;
    } else {
      mpfr_mul_d(count, count, val, round);
    }
  }

  // Calculate: 1/list.size()
  mpfr_t one, div, final;
  mpfr_inits2(precision, one, div, final, NULL);
  mpfr_set_ui(one, 1UL, round);
  mpfr_div_ui(div, one, list.size()-skipped, round);

  // Calculate: product(list) ^ (1/list.size())
  mpfr_pow(final, count, div, round);

  // Extract the final result.
  double result = mpfr_get_d(final, round);

  // And we're done!
  mpfr_clears(count, one, div, final, NULL);
  if (list.size() == skipped) return 0.0;  // result is 1.0 at this point.
  return result;
}

// Note: If a kernel/arch pair cannot run then it has a runtime of zero, so
// (correctly) treat that as a speedup of zero.
double calc_speedup(double a, double b, bool flip)
{
  if (flip) return calc_speedup(b, a, false);
  if (b == 0.0) return 0.0; // Sample didn't run.
  return a / b; // Both ran.
}
double calc_speedup(int a, int b, bool flip)
{
  return calc_speedup((double)a, (double)b, flip);
}
