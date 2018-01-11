/*
 * Various helper & utility routines.
 *
 * Author: Rob Lyerly
 * Date: 9/13/2015
 */

#ifndef _UTIL_H
#define _UTIL_H

namespace utility
{

/* Prediction utilities */
void set_threshold(float new_threshold);
bool within_threshold(float a, float b);
bool better_candidate(const std::pair<int, float>& a,
											const std::pair<int, float>& b);

/* Queue & prediction utilities */
bool is_available(size_t prediction_slot);
bool same_device(size_t q1, size_t q2);
std::string queue_sizes();
std::vector<size_t> alloc_to_index(struct resource_alloc alloc);
struct resource_alloc index_to_alloc(size_t index);
float get_prediction(size_t q, size_t j, size_t q_to_predict);
std::vector<size_t> get_candidates(Job* job);

};

#endif /* _UTIL_H */

