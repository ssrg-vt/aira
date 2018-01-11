/******************************************************************************/
/* Resource allocation policy interface.                                      */
/******************************************************************************/

#ifndef _POLICY_H
#define _POLICY_H

/* Available policies */

#define POLICIES \
	X(STATIC = 0, "static") \
	X(PARTITION, "partition") \
	X(ARCH_ADJ, "architecture adjustment") \
	X(PARTITION_AND_ADJ, "partition & architecture adjustment")

enum policy {
#define X(a, b) a,
POLICIES
#undef X
NUM_POLICIES
};

extern const char* policyNames[];

/*
 *
 */
class Policy
{
public:
	Policy() {};
	virtual ~Policy() {};
	virtual struct resource_alloc apply(float start,
																			std::vector<float>& predictions,
																			struct kernel_features* features,
																			float* predicted_end) = 0;
};

/*
 *
 */
class StaticPolicy : public Policy
{
	virtual struct resource_alloc apply(float start,
																			std::vector<float>& predictions,
																			struct kernel_features* features,
																			float* predicted_end);
};

/*
 *
 */
class PartitionPolicy : public Policy
{
public:
	virtual struct resource_alloc apply(float start,
																			std::vector<float>& predictions,
																			struct kernel_features* features,
																			float* predicted_end);
};

/*
 *
 */
class ArchAdjPolicy : public Policy
{
	virtual struct resource_alloc apply(float start,
																			std::vector<float>& predictions,
																			struct kernel_features* features,
																			float* predicted_end);
};

/*
 *
 */
class PartitionAndAdjPolicy : public Policy
{
	virtual struct resource_alloc apply(float start,
																			std::vector<float>& predictions,
																			struct kernel_features* features,
																			float* predicted_end);
};

#endif /* _POLICY_H */

