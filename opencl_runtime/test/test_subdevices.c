#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>

#include "cl_rt.h"

int verbose = 0;

void test_device(cl_runtime rt, struct cl_exec exec, cl_device_id in_dev)
{
	char desc[2048];
	size_t ret;

	clGetDeviceInfo(in_dev, CL_DEVICE_NAME, sizeof(desc), desc, &ret);
	printf("--> %s", desc);

	if(verbose)
	{
		clGetDeviceInfo(in_dev, CL_DEVICE_EXTENSIONS, sizeof(desc), desc, &ret);
		printf(" (extensions: %s)", desc);
		fflush(stdout);
	}

	//Create sub-device with 1/2 work units
	cl_int num_units = get_num_compute_units(rt, exec.platform, exec.device);
	cl_device_id out_dev = get_subdevice(rt, exec.platform, exec.device, num_units/2);
	cl_int num_out_units;
	clGetDeviceInfo(out_dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_out_units, &ret);

	if(num_out_units == (num_units/2))
		printf(": success!\n");
	else
		printf(": cannot subdivide\n");
}

void test_platforms()
{
	int i, j;
	char desc[256];
	struct cl_exec exec = { 0, 0, 1 };

	//Initialize the runtime
	cl_runtime rt = new_cl_runtime(false);
	printf("Testing %d platforms\n\n", get_num_platforms());
	for(i = 0; i < get_num_platforms(); i++)
	{
		exec.platform = i;
		clGetPlatformInfo(get_platform(rt, i), CL_PLATFORM_NAME, sizeof(desc), desc, NULL);
		printf("Testing devices for %s platform\n", desc);
		for(j = 0; j < get_num_devices(i); j++)
		{
			exec.device = j;
			test_device(rt, exec, get_device(rt, i, j));
		}
		printf("\n");
	}
	delete_cl_runtime(rt);
}

int main(int argc, char** argv)
{
	if(argc == 2 && !strncmp(argv[1], "-v", 2)) verbose = 1;
	test_platforms();
	return 0;
}

