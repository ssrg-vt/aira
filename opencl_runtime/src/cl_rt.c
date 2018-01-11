#include <stdlib.h>
#include <stdio.h>
#include <CL/cl.h>

#include "cl_rt.h"
#include "cl_error.h"
#include "print_info.h"

///////////////////////////////////////////////////////////////////////////////
// Internal definitions & state
///////////////////////////////////////////////////////////////////////////////

typedef struct platforms {
	cl_uint num_platforms;
	cl_platform_id* platforms;
} platforms;

typedef struct devices {
	cl_uint num_devices;
	cl_platform_id platform;
	cl_device_id* devices;
} devices;

typedef struct contexts {
	cl_uint num_contexts;
	cl_context* contexts;
} contexts;

typedef struct device_queue {
	cl_device_id id;
	cl_command_queue q;
} device_queue;

typedef struct device_queues {
	cl_uint num_queues;
	cl_context context;
	device_queue* queues;
} device_queues;

/* Handle for all runtime state */
struct _cl_runtime {
	bool initialized;
	platforms pf;
	devices* dv;
	contexts ctx;
	device_queues* qs;
};

/* OpenCL device fission extension handling in OpenCL 1.1 */
#if !defined(_ANDROID) && !defined(CL_VERSION_1_2)
#include <CL/cl_ext.h>
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define INIT_CL_EXT_FCN_PTR( name ) \
if(!pfn_##name) { \
	pfn_##name = (name##_fn) clGetExtensionFunctionAddress(#name); \
	if(!pfn_##name) { \
		printf("Could not get extension function pointer for: " #name "\n"); \
		exit(1); \
	} \
}
static clCreateSubDevicesEXT_fn pfn_clCreateSubDevicesEXT = NULL;
#else
#define INIT_CL_EXT_FCN_PTR( name )
#endif

/* Global library state */
static cl_uint num_platforms = 0;
static cl_uint* num_devices = 0;

#define BUILDLOG_SIZE 16384

///////////////////////////////////////////////////////////////////////////////
// Library initialization & teardown
///////////////////////////////////////////////////////////////////////////////

/*
 * Library constructor - initialize sub-device handling (if required) & check
 * for available platforms/devices.
 */
static void __attribute__((constructor))
init_ocl_rt()
{
	cl_uint i;
	cl_platform_id* ids;

	// Enable sub-device creation in OpenCL 1.1
	INIT_CL_EXT_FCN_PTR(clCreateSubDevicesEXT);

	// Query number of available platforms & devices
	OCLCHECK(clGetPlatformIDs(0, NULL, &num_platforms));
	ids = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
	num_devices = (cl_uint*)malloc(sizeof(cl_uint) * num_platforms);
	OCLCHECK(clGetPlatformIDs(num_platforms, ids, NULL));
	for(i = 0; i < num_platforms; i++)
		OCLCHECK(clGetDeviceIDs(ids[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices[i]));
	free(ids);
}

/*
 * Library destructor
 */
static void __attribute__((destructor))
dest_ocl_rt()
{
	free(num_devices);
}

///////////////////////////////////////////////////////////////////////////////
// Handle constructors & destructors
///////////////////////////////////////////////////////////////////////////////

/*
 * Setup the runtime environment, and if specified, initialize run queues for
 * all devices.
 */
cl_runtime new_cl_runtime(bool init_queues)
{
	cl_int i, j, err;
	cl_runtime rt = (cl_runtime)malloc(sizeof(struct _cl_runtime));

	// Initialize platforms
	rt->pf.num_platforms = num_platforms;
	rt->pf.platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
	OCLCHECK(clGetPlatformIDs(num_platforms, rt->pf.platforms, NULL));

	// Initialize devices
	rt->dv = (devices*)malloc(sizeof(devices) * num_platforms);
	for(i = 0; i < num_platforms; i++)
	{
		rt->dv[i].num_devices = num_devices[i];
		rt->dv[i].platform = rt->pf.platforms[i];
		rt->dv[i].devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices[i]);
		OCLCHECK(clGetDeviceIDs(rt->pf.platforms[i], CL_DEVICE_TYPE_ALL,
														num_devices[i], rt->dv[i].devices, NULL));
	}

	// Initialize contexts
	rt->ctx.num_contexts = num_platforms;
	rt->ctx.contexts = (cl_context*)malloc(sizeof(contexts) * num_platforms);
	for(i = 0; i < num_platforms; i++)
	{
		cl_context_properties props[] = {
			CL_CONTEXT_PLATFORM, (cl_context_properties)rt->pf.platforms[i], 0
		};
		rt->ctx.contexts[i] = clCreateContextFromType(props, CL_DEVICE_TYPE_ALL,
																									NULL, NULL, &err);
		OCLCHECK(err);
	}

	// Initialize device queues (if requested)
	rt->initialized = init_queues;
	if(init_queues)
	{
		rt->qs = (device_queues*)malloc(sizeof(device_queues) * num_platforms);
		for(i = 0; i < num_platforms; i++)
		{
			rt->qs[i].num_queues = num_devices[i];
			rt->qs[i].context = rt->ctx.contexts[i];
			rt->qs[i].queues = (device_queue*)malloc(sizeof(device_queue) * num_devices[i]);
			for(j = 0; j < num_devices[i]; j++)
			{
				rt->qs[i].queues[j].id = rt->dv[i].devices[j];
#ifdef CL_VERSION_2_0
				rt->qs[i].queues[j].q = clCreateCommandQueueWithProperties(rt->qs[i].context,
																																	 rt->qs[i].queues[j].id,
																																	 NULL, &err);
#else
				rt->qs[i].queues[j].q = clCreateCommandQueue(rt->qs[i].context,
																										 rt->qs[i].queues[j].id,
																										 0, &err);
#endif
				OCLCHECK(err);
			}
		}
	}

	return rt;
}

/*
 * Cleanup & free the runtime
 */
void delete_cl_runtime(cl_runtime runtime)
{
	int i, j;

	if(!runtime) return; // Semantically similar to free()

	// Tear down device queues
	if(runtime->initialized)
	{
		for(i = 0; i < num_platforms; i++)
		{
			for(j = 0; j < runtime->qs[i].num_queues; j++)
				OCLCHECK(clReleaseCommandQueue(runtime->qs[i].queues[j].q));
			free(runtime->qs[i].queues);
		}
		free(runtime->qs);
	}

	// Tear down contexts
	for(i = 0; i < num_platforms; i++)
		OCLCHECK(clReleaseContext(runtime->ctx.contexts[i]));
	free(runtime->ctx.contexts);

	// Tear down devices
	for(i = 0; i < num_platforms; i++)
		free(runtime->dv[i].devices);
	free(runtime->dv);

	// Tear down platforms & runtime
	free(runtime->pf.platforms);
	free(runtime);
}

///////////////////////////////////////////////////////////////////////////////
// Generic getters
///////////////////////////////////////////////////////////////////////////////

/*
 * Return the number of available platforms on the system.
 */
cl_uint get_num_platforms()
{
	return num_platforms;
}

/*
 * Return the number of available devices for the specified platform.
 */
cl_uint get_num_devices(cl_uint platform)
{
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);
	return num_devices[platform];
}

/*
 * Return the device type of the specified device.
 */
cl_device_type get_device_type(cl_runtime runtime, cl_uint platform, cl_uint device)
{
	cl_device_type dev_type;
	size_t ret_size;
	cl_device_id dev;

	if(!runtime) OCLERR("passed bad runtime argument");
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);
	if(num_devices[platform] <= device) OCLCHECK(CL_INVALID_DEVICE);

	dev = runtime->dv[platform].devices[device];
	OCLCHECK(clGetDeviceInfo(dev, CL_DEVICE_TYPE, sizeof(cl_device_type),
													 &dev_type, &ret_size));
	return dev_type;
}

/*
 * Return the number of compute units for the specified device on the specified
 * platform.
 */
cl_uint get_num_compute_units(cl_runtime runtime, cl_uint platform, cl_uint device)
{
	cl_uint num_units;
	size_t ret_size;
	cl_device_id dev;

	if(!runtime) OCLERR("passed bad runtime argument");
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);
	if(num_devices[platform] <= device) OCLCHECK(CL_INVALID_DEVICE);

	dev = runtime->dv[platform].devices[device];
	OCLCHECK(clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint),
													 &num_units, &ret_size));
	return num_units;
}

/*
 * Returns the number of available compute units for the specified device
 */
cl_uint get_num_compute_units_by_dev(cl_device_id dev_id)
{
	cl_uint num_units;
	OCLCHECK(clGetDeviceInfo(dev_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint),
		&num_units, NULL));
	return num_units;
}

/*
 * Get the maximum work-group size of a kernel for a given device
 */
size_t get_max_wg_size(cl_kernel kern, cl_device_id dev_id)
{
	size_t max_wg_size;
	OCLCHECK(clGetKernelWorkGroupInfo(kern, dev_id, CL_KERNEL_WORK_GROUP_SIZE,
																		sizeof(max_wg_size), &max_wg_size, NULL));
	return max_wg_size;
}

///////////////////////////////////////////////////////////////////////////////
// Generic getters
///////////////////////////////////////////////////////////////////////////////

/*
 * Return a platform ID for the requested platform number.
 */
cl_platform_id get_platform(cl_runtime runtime, cl_uint platform)
{
	if(!runtime) OCLERR("passed bad runtime argument");
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);

	return runtime->pf.platforms[platform];
}

/*
 * Return a device ID for the requested platform & device number.
 */
cl_device_id get_device(cl_runtime runtime, cl_uint platform, cl_uint device)
{
	if(!runtime) OCLERR("passed bad runtime argument");
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);
	if(num_devices[platform] <= device) OCLCHECK(CL_INVALID_DEVICE);

	return runtime->dv[platform].devices[device];
}

/*
 * Create a sub-device from the specified platform/device with the specified
 * number of compute units.
 */
cl_device_id get_subdevice(cl_runtime runtime, cl_uint platform, cl_uint device, cl_uint units)
{
	cl_device_type type;
	cl_device_id in_dev;

	if(!runtime) OCLERR("passed bad runtime argument");
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);
	if(num_devices[platform] <= device) OCLCHECK(CL_INVALID_DEVICE);

	in_dev = runtime->dv[platform].devices[device];
#ifdef _ANDROID
	// Android OpenCL library doesn't support sub-devices
	return in_dev;
#else
	cl_device_id out_dev;
	cl_uint num_ret = 0;
#endif

	// If no sub-device was requested, return the original device
	if(units == 0 || units == get_num_compute_units_by_dev(in_dev))
		return in_dev;

	// Failsafe -- GPUs & Accelerators don't support device fission.  When an
	// application requests device fission for an unsupported device, return
	// the original device
	type = get_device_type(runtime, platform, device);
	if(type == CL_DEVICE_TYPE_GPU || type == CL_DEVICE_TYPE_ACCELERATOR)
		return in_dev;

#if defined(CL_VERSION_1_2)
	//OpenCL v1.2 or later
	cl_device_partition_property pp[] = {
		CL_DEVICE_PARTITION_BY_COUNTS, units,
		CL_DEVICE_PARTITION_BY_COUNTS_LIST_END, 0
	};
	OCLCHECK(clCreateSubDevices(in_dev, pp, 1, &out_dev, &num_ret));
#else
	//OpenCL v1.1 (use extensions API)
	cl_device_partition_property_ext pp[] = {
		CL_DEVICE_PARTITION_BY_COUNTS_EXT, units,
		CL_PARTITION_BY_COUNTS_LIST_END_EXT, 0
	};
	OCLCHECK(pfn_clCreateSubDevicesEXT(in_dev, pp, 1, &out_dev, &num_ret));
#endif
	return out_dev;
}

/*
 * Return the corresponding context for the specified platform
 */
cl_context get_context_by_platform(cl_runtime runtime, cl_uint platform)
{
	if(!runtime) OCLERR("passed bad runtime argument");
	if(num_platforms <= platform) OCLCHECK(CL_INVALID_PLATFORM);

	return runtime->ctx.contexts[platform];
}

/*
 * Return the device's context
 *
 * NOTE: The application should NOT destroy this context, as it will be cleaned
 * up when the runtime is destroyed
 */
cl_context get_context_by_dev(cl_runtime runtime, cl_device_id dev_id)
{
	cl_uint i, j;

	if(!runtime) OCLERR("passed bad runtime argument");

	for(i = 0; i < num_platforms; i++)
		for(j = 0; j < num_devices[i]; j++)
			if(runtime->dv[i].devices[j] == dev_id)
				return runtime->ctx.contexts[i];
	return NULL;
}

/*
 * Return a device's command queue
 *
 * NOTE: The application should NOT destroy this command queue, as it will be
 * cleaned up when the runtime is destroyed
 */
cl_command_queue get_queue_by_dev(cl_runtime runtime, cl_device_id dev_id)
{
	int i, j;

	if(!runtime) OCLCHECK(CL_INVALID_VALUE);
	if(!runtime->initialized) OCLERR("cannot get queue for uninitialized runtime");

	for(i = 0; i < num_platforms; i++)
		for(j = 0; j < num_devices[i]; j++)
			if(runtime->dv[i].devices[j] == dev_id)
				return runtime->qs[i].queues[j].q;
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Convenience functions for general-purpose OpenCL programming
///////////////////////////////////////////////////////////////////////////////

/*
 * Build the program from the specified file for the specified device
 */
cl_program build_program_from_src(cl_runtime runtime, const char* fname,
																	cl_device_id dev)
{
	return build_program_with_args(runtime, fname, NULL, dev);
}

/*
 * Build the program from the specified file and with the specified arguments
 * for the specified device
 */
cl_program build_program_with_args(cl_runtime runtime, const char* fname,
																	 const char* args, cl_device_id dev)
{
	cl_program program;
	cl_context ctx;
	size_t fsize;
	char* src;
	cl_int err;

	if(!runtime) OCLERR("passed bad runtime argument");
	if(!fname) OCLERR("passed bad file name");

	ctx = get_context_by_dev(runtime, dev);

	// Read in source
	FILE* fp = fopen(fname, "r");
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	src = (char*)malloc(sizeof(char) * fsize + 1);
	if(fread(src, sizeof(char), fsize, fp) != fsize)
	{
		fprintf(stderr, "OpenCL runtime error: could not read in source\n");
		free(src);
		fclose(fp);
		OCLCHECK(CL_BUILD_PROGRAM_FAILURE);
	}
	src[fsize] = '\0';
	fclose(fp);
	program = clCreateProgramWithSource(ctx, 1, (const char**)(&src), &fsize, &err);
	OCLCHECK(err);

	// Build it!
	err = clBuildProgram(program, 1, &dev, args, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		size_t ret_size;
		char* buildlog = (char*)malloc(sizeof(char) * BUILDLOG_SIZE);
		OCLCHECK(clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
			sizeof(char) * BUILDLOG_SIZE, buildlog, &ret_size));
		fprintf(stderr, "<-- OpenCL build error -->\n\n%s\n", buildlog);
		if(ret_size == BUILDLOG_SIZE)
			fprintf(stderr, ".\n.\n.\nmore errors\n");
		free(buildlog);
		OCLCHECK(CL_BUILD_PROGRAM_FAILURE);
	}

	free(src);
	return program;
}

/*
 * Save a previously-compiled program for later use (avoid runtime compilation
 * overhead).
 */
size_t save_binary(cl_runtime runtime, cl_program program, cl_uint device,
									 const char* fname)
{
	cl_build_status status;
	cl_program_binary_type binary_type;

	cl_uint num_devs, i;
	cl_device_id* dev;
	size_t* binary_size;
	unsigned char** binary;

	size_t retsize;
	FILE* fp;

	if(!runtime) OCLERR("passed bad runtime argument");
	if(!fname) OCLERR("passed bad file name");

	// Get device number associated with program.  We have to do this because
	// some OpenCL runtimes (*cough* INTEL *cough*) REQUIRE you to get binaries
	// for all devices available in the context.
	OCLCHECK(clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint),
														&num_devs, &retsize));
	if(num_devs <= device) OCLERR("invalid device number\n");
	dev = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devs);
	OCLCHECK(clGetProgramInfo(program, CL_PROGRAM_DEVICES,
														sizeof(cl_device_id) * num_devs,
														dev, &retsize));

	// Check that program has been built into executable binary
	OCLCHECK(clGetProgramBuildInfo(program, dev[device], CL_PROGRAM_BUILD_STATUS,
																 sizeof(cl_build_status), &status, &retsize));
	if(status != CL_BUILD_SUCCESS)
		OCLERR("could not save binary as it has not been compiled\n");
	OCLCHECK(clGetProgramBuildInfo(program, dev[device], CL_PROGRAM_BINARY_TYPE,
																 sizeof(cl_program_binary_type), &binary_type,
																 &retsize));
	if(binary_type != CL_PROGRAM_BINARY_TYPE_EXECUTABLE)
		OCLERR("binary is not suitable for saving (make sure it has been compiled "
					 "& linked into an executable)\n");

	// Get binary size & data
	binary_size = (size_t*)malloc(sizeof(size_t) * num_devs);
	OCLCHECK(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
														sizeof(size_t) * num_devs,
														binary_size, &retsize));
	binary = (unsigned char**)malloc(sizeof(unsigned char*) * num_devs);
	for(i = 0; i < num_devs; i++)
		binary[i] = (unsigned char*)malloc(sizeof(unsigned char) * binary_size[i]);
	OCLCHECK(clGetProgramInfo(program, CL_PROGRAM_BINARIES,
														sizeof(unsigned char*) * num_devs,
														binary, &retsize));

	// Write data to file & clean up
	fp = fopen(fname, "w");
	retsize = fwrite(binary[device], sizeof(char), binary_size[device], fp);
	fclose(fp);

	for(i = 0; i < num_devs; i++)
		free(binary[i]);
	free(binary);
	free(binary_size);
	free(dev);
	return retsize;
}

/*
 * Load a previously-compiled binary for the specified device.
 */
cl_program load_binary(cl_runtime runtime, const char* fname, cl_device_id dev)
{
	FILE* fp;
	unsigned char* binary;
	size_t binary_size;
	cl_context ctx;
	cl_program program;
	cl_int load_status;
	cl_int err;

	if(!runtime) OCLERR("passed bad runtime argument");
	if(!fname) OCLERR("passed bad file name");

	ctx = get_context_by_dev(runtime, dev);

	// Read in binary
	fp = fopen(fname, "r");
	fseek(fp, 0, SEEK_END);
	binary_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	binary = (unsigned char*)malloc(sizeof(unsigned char) * binary_size);
	if(fread(binary, sizeof(char), binary_size, fp) != binary_size)
	{
		fprintf(stderr, "OpenCL runtime error: could not read in binary\n");
		free(binary);
		fclose(fp);
		OCLCHECK(CL_BUILD_PROGRAM_FAILURE);
	}
	fclose(fp);
	program = clCreateProgramWithBinary(ctx, 1, &dev, &binary_size, 
																			(const unsigned char**)&binary,
																			&load_status, &err);
	OCLCHECK(err);
	OCLCHECK(load_status);

	free(binary);
	return program;
}

