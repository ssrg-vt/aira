#ifndef _CL_ERROR_H
#define _CL_ERROR_H

/* Macro for printing an error message & quitting */
#define OCLERR( msg ) do { \
	fprintf(stderr, "OpenCL runtime error (%s, line %d): %s\n", \
		__FILE__, __LINE__, msg); \
	exit(1); \
} while(0);

/* Macro to check that the OpenCL operation completed successfully */
#define OCLCHECK( expr ) do { \
	cl_int __err = expr; \
	if(__err != CL_SUCCESS) { \
		fprintf(stderr, "OpenCL runtime error (%s, line %d): %s\n", \
			__FILE__, __LINE__, get_ocl_error(__err)); \
		exit(1); \
	} \
} while(0);

#endif /* _CL_ERROR_H */
