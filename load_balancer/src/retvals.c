#include "retvals.h"

/* Descriptions of return codes */
const char* retval_desc[] = {
#define X(a, b) b,
RETVALS
#undef X
};

