/*
 * Message Definitions - defines message structure for IPC.
 *
 * Author: Rob Lyerly
 * Dat: 8/8/2015
 */

#include <unistd.h>
#include <stdint.h>

#include "config.h"
#include "aira_definitions.h"

#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MAX_ARCHES 5

#define MESSAGE_TYPES \
	X(HW_REQUEST = 0, "hardware request") \
	X(HW_NOTIFY, "hardware notification") \
	X(HW_ASSIGN, "allocate resources") \
	X(KERNEL_FINISH, "kernel finished running") \
	X(GET_QUEUES, "get queue sizes") \
	X(RET_QUEUES, "return queue sizes") \
	X(CLR_QUEUES, "clear queues") \
	X(STOP_SERVER, "stop the daemon")

/* Types of messages */
enum message_type {
#define X(a, b) a,
MESSAGE_TYPES
#undef X
};

extern const char* message_type_str[];

/* Message format for communication between client & load balancer daemon */
struct message {
	pid_t sender_pid; // PID of process sending the message
	enum message_type type;	// Type of message being sent

	/* Message body, dependent on message type */
	union {
		struct kernel_features features; // HW_REQUEST
		struct resource_alloc alloc; // HW_NOTIFY & HW_ASSIGN
		int num_allocs[MAX_ARCHES]; // RET_TABLE
	} body;
};

/*
 * Encapsulates a connection, meaning the file-descriptor & message associated
 * with a connection.
 */
struct connection {
	int fd;
	struct message msg;
};

#endif /* _MESSAGE_H */

