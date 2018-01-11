#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "PowerMeasurement.h"

int main()
{
	size_t i, j;

	// Query info
	printf("Number of available managers: %lu\n", powerlib_num_managers());
	for(i = 0; i < powerlib_num_managers(); i++)
	{
		printf("Manager: %s, %lu device(s)\n", powerlib_manager_info(i),
																					 powerlib_num_devices(i));
		for(j = 0; j < powerlib_num_devices(i); j++)
		{
			printf("  %s (%s)\n", powerlib_device_info(i, j),
				(powerlib_device_supported(i, j) ? "supported" : "unsupported"));
		}
	}

	// Construct a handle
	printf("\nConstructing handle...");
	fflush(stdout);
	powerlib_t handle = powerlib_initialize();
	assert(handle);
	printf("success!\n");

	// Add all devices & monitor
	printf("Adding all devices...");
	int numAdded = powerlib_add_all_devices(handle);
	assert(numAdded >= 0);
	printf("success! Added %d device(s)\n", numAdded);

	printf("Monitoring/sleeping for 5 seconds (measuring power every 250ms)...");
	fflush(stdout);
	struct timespec period = {
		.tv_sec = 0,
		.tv_nsec = 250000000
	};
	assert(!powerlib_start_monitoring(handle, &period));
	assert(sleep(5) == 0);
	assert(!powerlib_stop_monitoring(handle));
	printf("success! Measured for %d periods\n", powerlib_num_periods_measured(handle));
	const char* summary = powerlib_summarize_measurements(handle);
	assert(summary);
	printf("\n%s\n", summary);

	// Clean up & exit
	printf("Removing all previously added devices...");
	assert(powerlib_remove_all_devices(handle) == numAdded);
	printf("success!\n");

	powerlib_shutdown(handle);
	return 0;
}
