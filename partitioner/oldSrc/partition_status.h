/*
 * partition_status.h
 *
 *  Created on: Jan 21, 2013
 *      Author: rlyerly
 */

#ifndef PARTITION_STATUS_H_
#define PARTITION_STATUS_H_

/*
 * Enumeration which describes the partitioning status of a function,
 * including if a function can or cannot be partitioned.
 */
enum status {
	//Function declaration statuses
	NOT_SELECTED_FOR_PARTITIONING,
	READY_FOR_PARTITIONING,
	PARTITIONED,
	NO_DEFINITION,
	CONTAINS_FUNC_CALLS_WO_DEFINITION,
	INVALID_INTERFACE,

	//Function call site statuses
	CALL_SITE_INITIALIZED,
	CALL_SITE_UPDATED,
	COULD_NOT_SANITIZE,
	INVALID_ARGUMENT_LIST,
};

#endif /* PARTITION_STATUS_H_ */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
