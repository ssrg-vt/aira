#include <assert.h>

#include "message.h"
#include "server/server.h"

///////////////////////////////////////////////////////////////////////////////
// Job implementation
///////////////////////////////////////////////////////////////////////////////

Job::Job(struct connection conn) :
	fd(conn.fd), client(conn.msg.sender_pid), predictions(prediction_slots)
{
	memset(&queued, 0, sizeof(struct timespec));
	memset(&start, 0, sizeof(struct timespec));
	memset(&end, 0, sizeof(struct timespec));
	if(conn.msg.type == HW_NOTIFY)
	{
		memset(&features, 0, sizeof(struct kernel_features));
		memcpy(&alloc, &conn.msg.body.alloc, sizeof(struct resource_alloc));
	}
	else if(conn.msg.type == HW_REQUEST)
	{
		memcpy(&features, &conn.msg.body.features, sizeof(struct kernel_features));
		memset(&alloc, 0, sizeof(struct resource_alloc));
	}
	else
		assert(false && "Shouldn't be in here!");
}

