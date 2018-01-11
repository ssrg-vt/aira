#include "message.h"

const char* message_type_str[] = {
#define X(a, b) b,
MESSAGE_TYPES
#undef X
};

