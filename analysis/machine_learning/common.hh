#ifndef _COMMON_HH
#define _COMMON_HH

#include "include/data.hh"
#include "include/eval.hh"

void standard_label_transform(DataManager &data, int knn_k,
                              bool speedup, bool prob,
                              std::set<int>& cut_labels);
void standard_feature_transform(DataManager &data, bool cut_empty,
                                bool scale_means, bool scale_ranges,
                                std::set<int>& cut_features);
void evaluation_report(const EvaluationManager &em);
void evaluation_single(const EvaluationManager &em, const std::string &name, const bool perf_predict);
void header_single();

#endif // _COMMON_HH
