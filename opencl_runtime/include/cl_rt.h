#include <stdbool.h>
#include <stdint.h>
#include <CL/cl.h>

#ifndef _CL_RT_H
#define _CL_RT_H

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Public definitions
///////////////////////////////////////////////////////////////////////////////

/* OpenCL runtime library handle */
typedef struct _cl_runtime* cl_runtime;

///////////////////////////////////////////////////////////////////////////////
// Library functions
///////////////////////////////////////////////////////////////////////////////

/* Handle constructors & destructors */
cl_runtime new_cl_runtime(bool init_queues);
void delete_cl_runtime(cl_runtime runtime);

/* Generic getters */
cl_uint get_num_platforms();
cl_uint get_num_devices(cl_uint platform);
cl_device_type get_device_type(cl_runtime runtime, cl_uint platform,
															 cl_uint device);
cl_uint get_num_compute_units(cl_runtime runtime, cl_uint platform,
															cl_uint device);
cl_uint get_num_compute_units_by_dev(cl_device_id dev_id);
size_t get_max_wg_size(cl_kernel kernel, cl_device_id dev_id);

/* Handle getters */
cl_platform_id get_platform(cl_runtime runtime, cl_uint platform);
cl_device_id get_device(cl_runtime runtime, cl_uint platform, cl_uint device);
cl_device_id get_subdevice(cl_runtime runtime, cl_uint platform,
													 cl_uint device, cl_uint units);
cl_context get_context_by_platform(cl_runtime runtime, cl_uint platform);
cl_context get_context_by_dev(cl_runtime runtime, cl_device_id dev_id);
cl_command_queue get_queue_by_dev(cl_runtime runtime,	cl_device_id dev_id);

/* Convenience functions for general-purpose OpenCL programming */
cl_program build_program_from_src(cl_runtime runtime, const char* fname,
																	cl_device_id dev);
cl_program build_program_with_args(cl_runtime runtime, const char* fname,
																	 const char* args, cl_device_id dev);
size_t save_binary(cl_runtime runtime, cl_program program, cl_uint device,
									 const char* fname);
cl_program load_binary(cl_runtime runtime, const char* fname,
											 cl_device_id dev);

/* Error handling */
const char* get_ocl_error(cl_int errcode);

#ifdef __cplusplus
}
#endif

#endif /* _CL_RT_H */

