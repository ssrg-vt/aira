#include <stdio.h>
#include <CL/cl.h>
#include "cl_rt.h"
#include "print_info.h"

int main(int argc, char** argv)
{
	cl_device_id dev;
	int i, j;
	unsigned long res;
	cl_runtime rt = new_cl_runtime(false);

	for(i = 0; i < get_num_platforms(); i++)
	{
		for(j = 0; j < get_num_devices(i); j++)
		{
			dev = get_device(rt, i, j);
			print_device_info(dev, false);
			clGetDeviceInfo(dev, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(cl_ulong), &res, NULL);
			printf("---\nTimer resolution: %lu\n---\n", res);
		}
	}
	delete_cl_runtime(rt);
	return 0;
}

