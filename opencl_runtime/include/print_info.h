#ifndef _PRINT_INFO_H
#define _PRINT_INFO_H

/* Platform information */
void print_platform_info(cl_platform_id platform);
void print_all_platform_info(cl_runtime runtime);

/* Device information */
void print_device_info(cl_device_id device, bool verbose);
void print_all_device_info(cl_runtime runtime, bool verbose);

#endif /* _PRINT_INFO_H */
