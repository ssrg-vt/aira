/*
 * Client-side communication - handles all communication with server.
 *
 * Author: Rob Lyerly
 * Date: 8/8/2015
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "aira_definitions.h"
#include "aira_runtime.h"
#include "message.h"
#include "ipc.h"
#include "retvals.h"

///////////////////////////////////////////////////////////////////////////////
// Definitions & state
///////////////////////////////////////////////////////////////////////////////

struct _aira_conn {
	bool open;
	client_channel channel;
	struct resource_alloc alloc;
};

static pid_t client_pid = -1;

///////////////////////////////////////////////////////////////////////////////
// Internal API
///////////////////////////////////////////////////////////////////////////////

/* Initialize the client-side communication */
static void __attribute__((constructor))
init_client()
{
	client_pid = getpid();
}

/*
 * Open a new connection to the server (if one has not been previously opened)
 * or re-open a previously established connection.
 */
static int establish_conn(aira_conn conn)
{
	if(!conn->open)
	{
		if(reopen_client_conn(conn->channel))
		{
#ifdef _CLIENT_VERBOSE
			fprintf(stderr, "Warning: could not reopen connection to server\n");
#endif
			return -1;
		}
		conn->open = true;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Client-side API implementation
///////////////////////////////////////////////////////////////////////////////

/*
 * Initializes a connection data structure.
 */
aira_conn aira_init_conn()
{
	aira_conn conn = (aira_conn)malloc(sizeof(struct _aira_conn));
	if(!conn) return NULL;

	conn->channel = open_client_channel(SOCKET_FILE);
	if(!conn->channel)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: could not open connection to server\n");
#endif
		free(conn);
    return NULL;
	}
	conn->open = true;
	conn->alloc.platform = 0;
	conn->alloc.device = 0;
	conn->alloc.compute_units = 1;

	return conn;
}

/*
 * Cleans-up & frees a connection data structure.
 */
void aira_free_conn(aira_conn conn)
{
  if(conn)
  {
		close_client_channel(conn->channel);
		free(conn);
  }
}

/*
 * Notify the load balancer that a kernel in the current application is going
 * to run on the specified resource allocation.
 */
void aira_notify(aira_conn conn, struct resource_alloc alloc)
{
	struct message msg;

	msg.sender_pid = client_pid;
	msg.type = HW_NOTIFY;
	msg.body.alloc = alloc;
	conn->alloc = alloc;

	if(establish_conn(conn))
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: could not open IPC channel\n");
#endif
		return;
	}

	if(client_send(conn->channel, &msg) != SUCCESS ||
		 client_receive(conn->channel, &msg) != SUCCESS)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: problem notifying server\n");
#endif
		return;
	}

	close_client_connection(conn->channel);
	conn->open = false;
}

/*
 * Query the load balancer for a resource allocation using the specified kernel
 * features.
 */
struct resource_alloc aira_alloc_resources(aira_conn conn,
																					 struct kernel_features* features)
{
	struct message msg;

	msg.sender_pid = client_pid;
	msg.type = HW_REQUEST;
	memcpy(&msg.body.features, features, sizeof(struct kernel_features));
	conn->alloc.platform = 0;
	conn->alloc.device = 0;
	conn->alloc.compute_units = 1;

	if(establish_conn(conn))
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: could not open IPC channel\n");
#endif
		return conn->alloc;
	}

	if(client_send(conn->channel, &msg) != SUCCESS)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: problem requesting allocation from daemon\n");
#endif
		return conn->alloc;
	}

	if(client_receive(conn->channel, &msg) != SUCCESS)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: problem receiving allocation from daemon\n");
#endif
		return conn->alloc;
	}

	close_client_connection(conn->channel);
	conn->open = false;
	conn->alloc = msg.body.alloc;
	return conn->alloc;
}

/*
 * Cleanup after a kernel has finished.
 */
void aira_kernel_finish(aira_conn conn)
{
	struct message msg;

	msg.sender_pid = client_pid;
	msg.type = KERNEL_FINISH;
	msg.body.alloc = conn->alloc;

	if(establish_conn(conn))
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: could not open IPC channel\n");
#endif
		return;
	}

	if(client_send(conn->channel, &msg) != SUCCESS)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: problem cleaning up with server\n");
#endif
		return;
	}

	close_client_connection(conn->channel);
	conn->open = false;
	return;	
}

/*
 * Get the current information regarding the runtime queues maintained by the
 * server.
 */
void aira_get_current_alloc(aira_conn conn, int* num_entries, int* entries)
{
	struct message msg;
	int success = 1;

	if(!entries)
	{
		*num_entries = MAX_ARCHES;
		return;
	}

	msg.sender_pid = client_pid;
	msg.type = GET_QUEUES;

	if(establish_conn(conn))
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: could not open IPC channel\n");
#endif
		success = 0;
	}

	if(success && client_send(conn->channel, &msg) != SUCCESS)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning: could not request resource usage from server\n");
#endif
		success = 0;
	}

	if(success && client_receive(conn->channel, &msg) != SUCCESS)
	{
#ifdef _CLIENT_VERBOSE
		fprintf(stderr, "Warning could not receive resource usage from server\n");
#endif
		success = 0;
	}

	if(success) {
		close_client_connection(conn->channel);
		conn->open = false;
	}

	int i;
	int num_to_populate = (*num_entries < MAX_ARCHES ? *num_entries : MAX_ARCHES);
	if(success)
		for(i = 0; i < num_to_populate; i++)
			entries[i] = msg.body.num_allocs[i];
	else
		for(i = 0; i < num_to_populate; i++)
			entries[i] = -1;

	return;
}

