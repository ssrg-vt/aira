/*
 * Performs communication with the server using multiple clients.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <omp.h>

#include "cl_rt.h"

#include "aira_runtime.h"
#include "kernels.h"

#define toNS( ts ) ((ts.tv_sec * 1000000000) + ts.tv_nsec)

int main(int argc, char** argv)
{
	struct kernel_features feats;
	struct resource_alloc alloc;
	struct timespec sleep, rem, start, end;
	cl_runtime rt;
	char tmp[8];
	unsigned i, j;

	printf("Please start the server and press [enter] to continue...");
	assert(fgets(tmp, sizeof(tmp), stdin));

	rt = new_cl_runtime(false);
	sleep.tv_sec = 0;
	sleep.tv_nsec = 500000000;
	aira_conn conn;

	// Test notification - server should not block client
	for(i = 0; i < get_num_platforms(); i++)
	{
		for(j = 0; j < get_num_devices(i); j++)
		{
			printf("Testing notification with 2 threads (%u/%u)...\n", i, j);

			alloc.platform = i;
			alloc.device = j;
			alloc.compute_units = get_num_compute_units(rt, i, j);

#pragma omp parallel num_threads(2) private(rem, start, end, conn)
			{
				conn = aira_init_conn();

				clock_gettime(CLOCK_MONOTONIC, &start); 
				aira_notify(conn, alloc);
				clock_gettime(CLOCK_MONOTONIC, &end);
				printf("%d took %lu NS to get allocation\n", omp_get_thread_num(),
							 toNS(end) - toNS(start));

				assert(!nanosleep(&sleep, &rem));

				aira_kernel_finish(conn);
				aira_free_conn(conn);
			}
		}
	}

	// Test resource allocation
	printf("Testing resource allocation request (2 x BT.A)...\n");
	feats.kernel = BT_A;
#pragma omp parallel num_threads(2) private(rem, start, end, conn)
	{
		conn = aira_init_conn();

		clock_gettime(CLOCK_MONOTONIC, &start);
		alloc = aira_alloc_resources(conn, &feats);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("%d took %lu NS to get allocation\n", omp_get_thread_num(),
					 toNS(end) - toNS(start));

		assert(!nanosleep(&sleep, &rem));

		aira_kernel_finish(conn);
		aira_free_conn(conn);
	}

	printf("Testing resource allocation request (%d x BT.A)...\n",
				 omp_get_num_threads());
#pragma omp parallel private(rem, start, end, conn)
	{
		conn = aira_init_conn();

		clock_gettime(CLOCK_MONOTONIC, &start);
		alloc = aira_alloc_resources(conn, &feats);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("%d took %lu NS to get allocation\n", omp_get_thread_num(),
					 toNS(end) - toNS(start));

		assert(!nanosleep(&sleep, &rem));

		aira_kernel_finish(conn);
		aira_free_conn(conn);
	}

	printf("Testing resource allocation request (2 X EP.C)...\n");
	feats.kernel = EP_C;
#pragma omp parallel num_threads(2) private(rem, start, end, conn)
	{
		conn = aira_init_conn();

		clock_gettime(CLOCK_MONOTONIC, &start);
		alloc = aira_alloc_resources(conn, &feats);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("%d took %lu NS to get allocation\n", omp_get_thread_num(),
					 toNS(end) - toNS(start));

		assert(!nanosleep(&sleep, &rem));

		aira_kernel_finish(conn);
		aira_free_conn(conn);
	}

	printf("Testing resource allocation request (BT.A + EP.C)...\n");
#pragma omp parallel num_threads(2) private(feats, rem, start, end, conn)
	{
		conn = aira_init_conn();

		switch(omp_get_thread_num()) {
		case 0:
			feats.kernel = BT_A;
			break;
		default:
			feats.kernel = EP_C;
			break;
		};
		clock_gettime(CLOCK_MONOTONIC, &start);
		alloc = aira_alloc_resources(conn, &feats);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("%d took %lu NS to get allocation\n", omp_get_thread_num(),
					 toNS(end) - toNS(start));

		assert(!nanosleep(&sleep, &rem));

		aira_kernel_finish(conn);
		aira_free_conn(conn);
	}

	delete_cl_runtime(rt);

	printf("Check the server's log for allocations/running kernels\n");
	return 0;
}

