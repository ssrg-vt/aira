/*
 * Performs communication with the server using only a single client.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include "cl_rt.h"

#include "aira_runtime.h"
#include "kernels.h"

int main(int argc, char** argv)
{
	struct kernel_features feats;
	struct resource_alloc alloc;
	struct timespec sleep, rem;
	aira_conn conn;
	cl_runtime rt;
	char tmp[8];
	unsigned i, j;

	printf("Please start the server and press [enter] to continue...");
	assert(fgets(tmp, sizeof(tmp), stdin));

	rt = new_cl_runtime(false);
	sleep.tv_sec = 0;
	sleep.tv_nsec = 500000000;
	conn = aira_init_conn();

	// Test notification
	for(i = 0; i < get_num_platforms(); i++)
	{
		for(j = 0; j < get_num_devices(i); j++)
		{
			printf("Testing notification (%u/%u)...\n", i, j);

			alloc.platform = i;
			alloc.device = j;
			alloc.compute_units = get_num_compute_units(rt, i, j);
			aira_notify(conn, alloc);

			assert(!nanosleep(&sleep, &rem));

			aira_kernel_finish(conn);
		}
	}

	// Test resource allocation
	printf("Testing resource allocation request (BT.A)...\n");
	feats.kernel = BT_A;
	alloc = aira_alloc_resources(conn, &feats);
	assert(!nanosleep(&sleep, &rem));
	aira_kernel_finish(conn);

	printf("Testing resource allocation request (EP.C)...\n");
	feats.kernel = EP_C;
	alloc = aira_alloc_resources(conn, &feats);
	assert(!nanosleep(&sleep, &rem));
	aira_kernel_finish(conn);

	aira_free_conn(conn);
	delete_cl_runtime(rt);

	printf("Check the server's log for allocations/running kernels\n");
	return 0;
}

