/*****************************************************************************/
/* Runtime Load-Balancer (Server) Interface                                  */
/*                                                                           */
/* This file describes the interface to the runtime load-balancer, i.e. the  */
/* user-space server that handles requests for access to heterogeneous	     */
/* hardware.  This includes defining filenames corresponding to the server's */
/* PID and a Unix socket file for IPC with the server.                       */
/*****************************************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <ctime>
#include <unistd.h>

#include "cl_rt.h"

#include "aira_definitions.h"
#include "config.h"

#include "server/job.h"
#include "server/prediction.h"
#include "server/hw_queue_config.h"
#include "server/hw_queue.h"
#include "server/config_parser.h"

/* Hardcoded per-system information */
#include "server/system.h"

/* Filename which contains the server's PID */
#define SERVER_PID_FILE "/var/run/aira-lb.pid"

/* Signals */
#define EXIT_SIG SIGINT /* Cleanup & terminate */
#define CLEAR_SIG SIGUSR1 /* Clear run queues */

/* Hardware queues & OpenCL runtime */
extern std::vector<HWQueue*> queues;
extern cl_runtime cl_rt; 

#endif /* _SERVER_H */

