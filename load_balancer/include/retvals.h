/*
 * Return codes for API functions.
 *
 * Author: Rob Lyerly
 * Date: 8/8/2015
 */

#ifndef _RETVALS_H
#define _RETVALS_H

#define RETVALS \
	X(SUCCESS = 0, "success") \
	X(NOT_ROOT, "not running server as root") \
	X(SERVER_RUNNING, "detected previous server instance") \
	X(PID_FILE_STORE_ERR, "could not store PID to file") \
	X(IPC_SETUP_ERR, "could not set up IPC channel") \
	X(SIGNAL_SETUP_ERR, "could not set up signals") \
	X(MODEL_INIT_ERR, "could not initialize performance models") \
	X(OCL_INIT_ERR, "could not initialize OpenCL runtime") \
	X(SERVER_SETUP_ERR, "could not initialize server") \
	X(IPC_CLEANUP_ERR, "could not clean up IPC channel") \
	X(CLEANUP_ERR, "clean up error") \
	X(SERVER_STOP_ERR, "could not successfully stop server") \
	X(BAD_IPC_CHANNEL, "bad IPC channel") \
	X(IPC_RECV_ERR, "IPC receive error") \
	X(IPC_SEND_ERR, "IPC send error") \
	X(RECV_ERR, "message receive error") \
	X(ALLOC_ERR, "allocation error") \
	X(FAILURE = 255, "general failure")

/* Return codes */
enum retval {
#define X(a, b) a,
RETVALS
#undef X
};

/* Text description of return codes */
extern const char* retval_desc[];

/* Macro to check return values */
#define CHECK_ERR( expr ) do { \
	int _retval = expr; \
	if(_retval != SUCCESS) \
	{ \
		fprintf(stderr, "Server error: %s\n", retval_desc[_retval]); \
		exit(_retval); \
	} \
} while(0)

#endif /* _RETVALS_H */

