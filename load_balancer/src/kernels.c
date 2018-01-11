#include "kernels.h"

const char* npb_kernel_names[] = {
#define X(a, b) b,
NPB_KERNELS
#undef X
};

