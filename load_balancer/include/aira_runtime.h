/* 
 * AIRA Load Balancing Library
 *
 * This library allows the application to communicate with the load balancer
 * daemon to coordinate access to shared heterogeneous resources.
 *
 * Author: Rob Lyerly
 * Date: 7/8/2015
 */

#ifndef _AIRA_RUNTIME
#define _AIRA_RUNTIME

#include "aira_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _aira_conn* aira_conn;

/*
 * Initialize data for a connection to the load-balancer.  Can be used by a
 * single process to get multiple connections to the server.
 *
 * @return a connection handle used for communication with server or NULL if
 *         something went wrong
 */
aira_conn aira_init_conn();

/*
 * Frees a connection to the load-balancer.  Should only be called after
 * the process has finished ALL communications with the server (e.g. in
 * a destructor).
 *
 * @param conn a previously opened connection to the load-balancer.
 */
void aira_free_conn(aira_conn conn);

/*
 * Notify the load balancer that the application is going to run a compute
 * kernel on the specifed resource allocation.
 *
 * @param alloc a resource allocation on which the application will execute a
 *              compute kernel
 * @return a connection handle used for further communication with server
 */
void aira_notify(aira_conn conn, struct resource_alloc alloc);

/*
 * Get a resource allocation from the load balancer for a kernel based on the
 * specified kernel features and external workload.
 *
 * @param conn a previously opened connection
 * @param features feature vector describing the kernel to be executed
 * @return a resource allocation from the load balancer
 */
struct resource_alloc aira_alloc_resources(aira_conn conn,
																					 struct kernel_features* features);

/*
 * Cleanup after a kernel finishes.
 */
void aira_kernel_finish(aira_conn conn);

/*
 * Get the number of resource allocations for all devices in the platform.
 * Application must pass storage (and indicate how many entries are available)
 * which will be filled by the library.  If user passes a null pointer,
 * num_entries will be populated with the number of total entries that can be
 * populated by the library.
 *
 * @param num_entries number of entries available for populating in entries
 * @param entries vector of ints (storage provided by application)
 */
// TODO get num running & num queued from server
void aira_get_current_alloc(aira_conn conn, int* num_entries, int* entries);

#ifdef __cplusplus
}
#endif

#endif /* _AIRA_RUNTIME */

