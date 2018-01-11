#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <time.h>

#include <CL/cl.h>
#include "cl_rt.h"

#define toS( ts ) ((double)ts.tv_sec + (((double)ts.tv_nsec) / 1000000000.0))

static const char* kernel_file = NULL;
static const char* outfile = "compiled.pcl";
static const char* args = "";
static cl_uint platform = 0;
static cl_uint device = 0;
static const char* argstring = "hi:o:a:p:d:";
static const char* help =
"precompile - pre-compile OpenCL kernel(s) for later loading\n\n"

"Usage: precompile [ OPTIONS ]\n"
"Options:\n"
"  -h      : print help & exit\n"
"  -i file : file containing OpenCL kernels (REQUIRED)\n"
"  -o file : output file containing compiled binary\n"
"  -a args : arguments for compilation (include all arguments in single double-quoted string)\n"
"  -p num  : platform of device for which to compile\n"
"  -d dev  : device (of the specified platform) for which to compile\n";

void print_help()
{
	printf("%s", help);
}

void parse_args(int argc, char** argv)
{
	int c;
	while((c = getopt(argc, argv, argstring)) != -1)
	{
		switch(c)
		{
		case 'h':
			print_help();
			exit(0);
			break;
		case 'i':
			kernel_file = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'a':
			args = optarg;
			break;
		case 'p':
			platform = atoi(optarg);
			break;
		case 'd':
			device = atoi(optarg);
			break;
		default:
			printf("Warning: invalid argument '%c'\n", c);
			break;
		}
	}

	// Sanity checks
	assert(kernel_file);
	assert(0 <= platform && platform <= get_num_platforms());
	assert(0 <= device && device <= get_num_devices(platform));
}

int main(int argc, char** argv)
{
	cl_runtime rt;
	cl_device_id dev;
	cl_program prog;
	size_t write_size;
	struct timespec start, end;

	parse_args(argc, argv);

	clock_gettime(CLOCK_MONOTONIC, &start);

	printf(" -> Setting up OpenCL runtime...");
	fflush(stdout);
	rt = new_cl_runtime(true);
	printf("success!\n");

	printf(" -> Getting specified device...");
	fflush(stdout);
	dev = get_device(rt, platform, device);
	printf("success!\n");

	printf(" -> Building program with source '%s' and arguments '%s'...",
				 kernel_file, args);
	fflush(stdout);
	prog = build_program_with_args(rt, kernel_file, args, dev);
	printf("success!\n");

	printf(" -> Saving binary to '%s'...", outfile);
	fflush(stdout);
	write_size = save_binary(rt, prog, device, outfile);
	printf("success! Wrote %lu bytes.\n", write_size);

	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("Compilation took %4.3f seconds\n", toS(end) - toS(start));

	clReleaseProgram(prog);
	delete_cl_runtime(rt);
	return 0;
}
