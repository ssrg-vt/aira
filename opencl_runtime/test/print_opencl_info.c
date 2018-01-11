///////////////////////////////////////////////////////////////////////////////
// Print OpenCL Info                                                         //
//                                                                           //
// Print OpenCL information regarding available platforms & devices in       //
// either a human-readable format or to a configuration file for a test      //
// harness to consume.                                                       //
//                                                                           //
// In terms of config file output, the file is of the form:                  //
//                                                                           //
// x                                                                         //
// 0:a,1:b,...                                                               //
// ...                                                                       //
//                                                                           //
// In other words:                                                           //
//   1. The 1st line contains the number of platforms                        //
//   2. Each additional line corresponds to a platform.  It enumerates the   //
//      devices and their number of work units in a colon-separated pair     //
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <CL/cl.h>

#include "cl_rt.h"
#include "print_info.h"

int main(int argc, char** argv)
{
	cl_runtime rt = new_cl_runtime(false);
	if(argc > 1 && !strcmp(argv[1], "--tofile"))
	{
		int num_platforms = get_num_platforms(rt);
		int num_devices;
		assert(num_platforms >= 0);

		printf("%d\n", num_platforms);

		int i, j;
		for(i = 0; i < num_platforms; i++)
		{
			num_devices = get_num_devices(i);
			assert(num_devices >= 0);

			char device_str[64] = "";
			char base_str[8];

			sprintf(base_str, "%d:%u", 0, get_num_compute_units(rt, i, 0));
			strcat(device_str, base_str);

			for(j = 1; j < num_devices; j++)
			{
				sprintf(base_str, ",%d:%u", j, get_num_compute_units(rt, i, j));
				strcat(device_str, base_str);
			}
			printf("%s\n", device_str);
		}
	}
	else
	{
		print_all_platform_info(rt);
		printf("\n");
		print_all_device_info(rt, true);
	}
	delete_cl_runtime(rt);

	return 0;
}

