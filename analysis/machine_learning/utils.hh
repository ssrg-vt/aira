#ifndef _UTILS_HH
#define _UTILS_HH

#include "data.hh"
#include "predict.hh"
#include "ml_data.hh"
#include "nn.hh"
#include "timer.hh"
#include <iomanip>

typedef enum { X86, TILERA, GPU, NUM_ARCHES } arch;

void evaluate_neural_network(const RegressionManager &nn,
                             const DataManager &test, bool speedup);
arch find_best_arch(const Mat &label, bool speedup);
const char *arch_str(arch a);
void set_default_precision(std::ostream &s);
void title(std::ostream &s, const std::string &msg);
void spacer(std::ostream &s);
void print_arches(std::ostream &s, const char *prestr, const char *poststr);

// Parsing helper functions.
int parse_int(const char *str);
double parse_float(const char *str);
Col32D parse_layers(const char *str);
std::set<int> parse_comma_list(const char *str);

// Calculate speedups while avoid divide-by-zero.
double average(const std::vector<double> &list, bool skip_blanks);
double geomean(const std::vector<double> &list, bool skip_blanks);
double calc_speedup(double a, double b, bool flip=false);
double calc_speedup(int a, int b, bool flip=false);

static inline bool approx(double a, double b) {
  double bmin = b - 0.001;
  double bmax = b + 0.001;
  return ((a >= bmin) && (a <= bmax));
}

#endif // _UTILS_HH
