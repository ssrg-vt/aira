#ifndef _HW_QUEUE_CONFIG_H
#define _HW_QUEUE_CONFIG_H

/*
 * Because of the complexity of configuring hardware queues, use an object to
 * contain all the gory details.
 */
class HWQueueConfig {
public:
	/* Device information */
	size_t platform;
	size_t device;
	size_t computeUnits;

	/* Execution configuration */
	size_t maxRunning;
	bool dynamicPartitioning;
};

#endif /* _HW_QUEUE_CONFIG_H */

