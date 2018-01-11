///////////////////////////////////////////////////////////////////////////////
// Print OpenCL runtime information
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <CL/cl.h>

#include "cl_rt.h"
#include "cl_error.h"
#include "print_info.h"

#define DESC_SIZE 256

/*
 * Print a platform's info
 */
void print_platform_info(cl_platform_id platform)
{
	char desc[DESC_SIZE];

	OCLCHECK(clGetPlatformInfo(platform, CL_PLATFORM_NAME, DESC_SIZE, desc, NULL));
	printf("Platform: %s (", desc);

	OCLCHECK(clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, DESC_SIZE, desc, NULL));
	printf("%s)\n", desc);

	OCLCHECK(clGetPlatformInfo(platform, CL_PLATFORM_VERSION, DESC_SIZE, desc, NULL));
	printf("Version : %s\n", desc);
}

/*
 * Print all platforms' info
 */
void print_all_platform_info(cl_runtime runtime)
{
	int i;
	for(i = 0; i < get_num_platforms(); i++)
	{
		print_platform_info(get_platform(runtime, i));
		printf("\n");
	}
}

/*
 * Print a device's info
 */
void print_device_info(cl_device_id device, bool verbose)
{
	cl_device_type type;
	cl_uint uint_val;
  cl_ulong ulong_val;
	size_t size_val, ret_size;
	char desc[DESC_SIZE];
	
	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_NAME, DESC_SIZE, desc, &ret_size));
	printf("Device: %s (", desc);
	
	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_VENDOR, DESC_SIZE, desc, &ret_size));
	printf("%s)\n", desc);
	
	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type),
		&type, &ret_size));
	printf("Device type: ");
	switch(type) {
	case CL_DEVICE_TYPE_CPU:
		printf("CPU\n");
		break;
	case CL_DEVICE_TYPE_GPU:
		printf("GPU\n");
		break;
	case CL_DEVICE_TYPE_ACCELERATOR:
		printf("Accelerator\n");
		break;
	case CL_DEVICE_TYPE_DEFAULT:
		printf("Default\n");
		break;
#ifdef CL_VERSION_1_2
	case CL_DEVICE_TYPE_CUSTOM:
		printf("Custom\n");
		break;
#endif
	default:
		printf("N/A (unknown)\n");
		break;
	}
 
	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint),
		&uint_val, &ret_size));
	printf("Number of compute units: %u\n", uint_val);
 
	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint),
		&uint_val, &ret_size));
	printf("Clock speed: %u MHz\n", uint_val);

	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong),
		&ulong_val, &ret_size));
#ifdef _ANDROID
	printf("Global memory size: %llu\n", ulong_val);
#else
	printf("Global memory size: %lu\n", ulong_val);
#endif
	OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong),
		&ulong_val, &ret_size));
#ifdef _ANDROID
	printf("Local memory size: %llu\n", ulong_val);
#else
	printf("Local memory size: %lu\n", ulong_val);
#endif
 
	//TODO print verbose information
	if(verbose)
	{
		OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
			sizeof(size_val), &size_val, &ret_size));
#ifdef _ANDROID
		printf("Maximum number of work items in a work group: %u\n", size_val);
#else
		printf("Maximum number of work items in a work group: %lu\n", size_val);
#endif

		OCLCHECK(clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
			sizeof(ulong_val), &ulong_val, &ret_size));
#ifdef _ANDROID
		printf("Maximum memory allocation size: %llu\n", ulong_val);
#else
		printf("Maximum memory allocation size: %lu\n", ulong_val);
#endif
	}
}

/*
 * Print all devices' info
 */
void print_all_device_info(cl_runtime runtime, bool verbose)
{
	int i, j, cur_dev = 0;
	char desc[DESC_SIZE];
	for(i = 0; i < get_num_platforms(); i++)
	{
		OCLCHECK(clGetPlatformInfo(get_platform(runtime, i), CL_PLATFORM_NAME, DESC_SIZE, desc, NULL));
		printf("Platform: %s\n", desc);

		for(j = 0; j < get_num_devices(i); j++)
		{
			printf("%d) ", cur_dev);
			print_device_info(get_device(runtime, i, j), verbose);
			cur_dev++;
			printf("\n");
		}
	}
}

