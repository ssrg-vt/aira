/*
 * Implementation of load balancer daemon.
 *
 * Author: Rob Lyerly
 * Date: 9/11/2015
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

/* Load-balancer includes */
#include "kernels.h"
#include "message.h"
#include "retvals.h"
#include "ipc.h"

/* Server-specific includes */
#include "server/server.h"
#include "server/util.h"

///////////////////////////////////////////////////////////////////////////////
// Server state
///////////////////////////////////////////////////////////////////////////////

const char* help_text = "aira-lb - heterogeneous load-balancing daemon\n\n"

"A server which listens for and responds to requests for a resource allocation"
" on which to execute a kernel.  It accepts kernel features from the client"
" and allocates resources based on hardware configuration parameters. See the"
" configuration tool in utility for help on how to configure the server.\n\n"

"Usage: ./aira-lb [OPTIONS]\n\n"

"Options:\n"
"  -h                : Print this help\n"
"  -m model file     : File specifying a model for estimating performance\n"
"  -t transform file : File specifying transformations to apply to features\n"
"  -p predictor      : Type of predictor - see below\n"
"  -c config file    : File containing queue configuration information\n\n"

"Valid predictors:\n"
"  nn           : use an artificial neural network to make predictions\n"
"  always-cpu   : always \"predict\" applications run fastest on the CPU\n"
"  always-gpu   : always \"predict\" applications run fastest on the GPU\n"
"  exact-rt     : use hard-coded static runtimes\n"
"  exact-energy : use hard-coded static energy consumptions\n"
"  exact-edp    : use both hard-coded static runtimes & energy consumptions to calculate the energy-delay product\n\n";

/*
 * Hardware queues.  Make global so they can be accessed by utility
 * functions
 */
std::vector<HWQueue*> queues;

/*
 * OpenCL runtime.  Make global so it can be used by predictors.
 */
cl_runtime cl_rt = NULL;

/* Process & IPC state */
static pid_t server_pid = 0;
static server_channel channel = NULL;
static volatile sig_atomic_t cleanup_flag = 0;

/* Configuration */
static std::string model_fn = "model.xml";
static std::string transform_fn = "trans.xml";
static std::string config_fn = "n/a";
static enum predictor predictor_type = NN;
static Predictor* predictor = NULL;

/* Serving statistics */
#ifdef _SERVER_STATISTICS
static size_t numRequestsServed = 0;
static size_t numNotifies = 0;
static size_t numAssigns = 0;
static size_t numReleases = 0;
static size_t numGetTables = 0;
static size_t numClears = 0;

static unsigned long long assignTime = 0;
static unsigned long long releaseTime = 0;
#endif

///////////////////////////////////////////////////////////////////////////////
// Function prototypes 
///////////////////////////////////////////////////////////////////////////////

/* Utility */
static int parse_args(int argc, char** argv);
static void print_configuration();
static inline int start_job(Job& job);
static int adjust_queues();

/* Initialization */
static int initialize();
static int check_prev_server();
static int check_root();
static int store_pid();
static int setup_signals();
static int initialize_queues();

/* Main functionality */
static int handle_requests();
static int notify_resources(struct connection& conn);
static int assign_resources(struct connection& conn);
static int release_resources(struct connection& conn);
static int send_queues(struct connection& conn);
static int clear_queues(struct connection& conn);

/* Cleanup */
static int cleanup();

/* Signal handlers */
static void exit_sig(int sig);
static void clear_sig(int sig);

///////////////////////////////////////////////////////////////////////////////
// Entry point
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	CHECK_ERR(parse_args(argc, argv));
	CHECK_ERR(initialize());
	CHECK_ERR(handle_requests());
	CHECK_ERR(cleanup());

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////////////////////////////

/*
 * Parse command-line arguments to configure the server.
 */
static int parse_args(int argc, char** argv)
{
	int arg = 0;

	while((arg = getopt(argc, argv, "hm:t:p:c:")) != -1)
	{
		switch(arg) {
		case 'h':
			printf("%s", help_text);
			exit(0);
		case 'm':
			model_fn = optarg;
			break;
		case 't':
			transform_fn = optarg;
			break;
		case 'p':
			if(!strcmp("nn", optarg))
				predictor_type = NN;
			else if(!strcmp("always-cpu", optarg))
				predictor_type = ALWAYS_CPU;
			else if(!strcmp("always-gpu", optarg))
				predictor_type = ALWAYS_GPU;
			else if(!strcmp("exact-rt", optarg))
				predictor_type = EXACT_RT;
			else if(!strcmp("exact-energy", optarg))
				predictor_type = EXACT_ENERGY;
			else if(!strcmp("exact-edp", optarg))
				predictor_type= EXACT_EDP;
			else
			{
				predictor_type = NN;
				printf("Unknown predictor '%s', reverting to 'nn'\n", optarg);
			}
			break;
		case 'c':
			config_fn = optarg;
			break;
		default:
			fprintf(stderr, "Unknown argument %c\n", arg);
			return SERVER_SETUP_ERR;
		}
	}

	return SUCCESS;
}

/*
 * Print the server's runtime configuration.
 */
static void print_configuration()
{
	printf("\n*** Server Configuration ***\n");
	printf("Model file: %s\n", model_fn.c_str());
	printf("Transform file: %s\n", transform_fn.c_str());
	printf("Predictor type: %s\n", predictorNames[predictor_type]);
	printf("Using %lu device(s):\n", queues.size());
	for(HWQueue* q : queues)
		q->printConfiguration();
	printf("\n");
}

/*
 * Start the specified job by notifying the client it can begin running.
 */
static inline int start_job(Job& job)
{
	struct connection conn;

	conn.fd = job.fd;
	conn.msg.sender_pid = server_pid;
	conn.msg.type = HW_ASSIGN;
	conn.msg.body.alloc = job.alloc;

	CHECK_ERR(server_send(&conn));
	CHECK_ERR(server_close_connection(&conn));

	return SUCCESS;
}

/*
 * Adjust hardware queues by re-distributing jobs (if possible) and by
 * changing the number of logical devices in the system (to either increase
 * throughput or increase the performance of a single device).
 */
static int adjust_queues()
{
	// TODO
#ifdef _SERVER_VERBOSE
	printf(", queues: %s\n", utility::queue_sizes().c_str());
#endif
	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

/*
 * Perform all steps necessary to initialize the server, including storing the
 * PID of this process, opening a socket for IPC and setting up signals for
 * exiting the process.
 */
static int initialize()
{
	server_pid = getpid();
	CHECK_ERR(check_prev_server());
	CHECK_ERR(check_root());
	CHECK_ERR(store_pid());
	channel = open_server_channel(SOCKET_FILE);
	CHECK_ERR(!channel ? IPC_SETUP_ERR : SUCCESS);
	CHECK_ERR(setup_signals());
	CHECK_ERR(initialize_queues());

	switch(predictor_type)
	{
	case NN:
		predictor = new NeuralNetPredictor(model_fn, transform_fn);
		break;
	case ALWAYS_CPU:
		predictor = new AlwaysCPU();
		break;
	case ALWAYS_GPU:
		predictor = new AlwaysGPU();
		break;
	case EXACT_RT:
		predictor = new ExactRuntime();
		break;
	case EXACT_ENERGY:
		predictor = new ExactEnergy();
		break;
	case EXACT_EDP:
		predictor = new ExactEDP();
	default:
		assert(false && "Shouldn't be in here...\n");
	}

	print_configuration();

	return SUCCESS;
}

/*
 * Check to make sure another instance of the server is not already running.
 */
static int check_prev_server()
{
	if(!access(SERVER_PID_FILE, F_OK))
	{
		FILE* fp = fopen(SERVER_PID_FILE, "r");
		pid_t serv_pid = 0;
		if(!fp || (fscanf(fp, "%d", &serv_pid) < 1))
		{	
			fprintf(stderr, "Found existing server PID file (could not read PID)\n");
			return SERVER_RUNNING;
		}

		fprintf(stderr, "Existing server running (pid %d)\n", serv_pid);
		fclose(fp);
		return SERVER_RUNNING;
	}

	return SUCCESS;
}

/*
 * Make sure the server was started with root privileges.  This is necessary to
 * create the PID & socket file, and to change the permissions so that user-land
 * applications can communicate with the server.
 */
static int check_root()
{
	if(geteuid() != 0)
		return NOT_ROOT;
	else
		return SUCCESS;
}

/*
 * Save the PID of the server process (this process) into a file for other
 * processes to retrieve.
 */
static int store_pid()
{
	int retval = 0;
	pid_t my_pid = getpid();

	// Open file
	FILE* pid_file = fopen(SERVER_PID_FILE, "w");
	if(!pid_file)
	{
		perror("Error opening PID file");
		return PID_FILE_STORE_ERR;
	}

	// Change file permissions
	int pid_fd = -1;
	if((pid_fd = fileno(pid_file)) == -1)
	{
		perror("Bad file descriptor");
		return PID_FILE_STORE_ERR;
	}

	mode_t mode = umask(0);
	mode |= S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	if(fchmod(pid_fd, mode) == -1)
	{
		perror("Error setting file permissions");
		return PID_FILE_STORE_ERR;
	}

	// Write PID
	retval = fprintf(pid_file, "%d\n", my_pid);
	if(retval < 0)
	{
		fprintf(stderr, "Could not write the PID to file\n");
		return PID_FILE_STORE_ERR;
	}

	// Close file
	retval = fclose(pid_file);
	if(retval)
	{
		perror("Error closing PID file");
		return PID_FILE_STORE_ERR;
	}

	return SUCCESS;
}

/*
 * Setup signal handling.  The server can be told to shutdown by sending it
 * EXIT_SIG and be reset to a clean state using CLEAR_SIG.
 */
static int setup_signals()
{
	struct sigaction exit;
	struct sigaction clear;

	exit.sa_handler = exit_sig;
	exit.sa_flags = 0;
	sigemptyset(&exit.sa_mask);
	if(sigaction(EXIT_SIG, &exit, NULL) < 0)
	{
		perror("Could not register signal handler: exit signal");
		return SIGNAL_SETUP_ERR;
	}

	clear.sa_handler = clear_sig;
	clear.sa_flags = 0;
	sigemptyset(&clear.sa_mask);
	if(sigaction(CLEAR_SIG, &clear, NULL) < 0)
	{
		perror("Could not register signal handler: clear signal");
		return SIGNAL_SETUP_ERR;
	}

	return SUCCESS;
}

/*
 * Initialize the HW queues.  Utilize the OpenCL runtime to query the system
 * for available devices.
 */
static int initialize_queues()
{
	cl_rt = new_cl_runtime(false);

	if(config_fn != "n/a") // User supplied configuration file
	{
		std::vector<HWQueueConfig*> configs = ConfigParser::parseConfig(config_fn);
		queues.resize(0);
		for(HWQueueConfig* config : configs)
		{
			switch(get_device_type(cl_rt, config->platform, config->device))
			{
			case CL_DEVICE_TYPE_CPU:
				queues.push_back(new CPUQueue(config));
				break;
			case CL_DEVICE_TYPE_GPU:
				queues.push_back(new GPUQueue(config));
				break;
			default:
				assert(false && "unsupported device type!");
			}
		}
	}
	else // Query the OpenCL runtime for device information
	{
		size_t numDevices = 0;
		for(size_t i = 0; i < get_num_platforms(); i++)
			numDevices += get_num_devices(i);
		assert(numDevices < MAX_ARCHES && "found too many devices!");

		HWQueueConfig* config;
		size_t devIndex = 0;
		queues.resize(numDevices);
		for(size_t i = 0; i < get_num_platforms(); i++)
		{
			for(size_t j = 0; j < get_num_devices(i); j++, devIndex++)
			{
				config = new HWQueueConfig();
				config->platform = i;
				config->device = j;
				config->computeUnits = get_num_compute_units(cl_rt, i, j);
				config->maxRunning = 1;
				config->dynamicPartitioning = false;
				switch(get_device_type(cl_rt, i, j))
				{
				case CL_DEVICE_TYPE_CPU:
					queues[devIndex] = new CPUQueue(config);
					break;
				case CL_DEVICE_TYPE_GPU:
					queues[devIndex] = new GPUQueue(config);
					break;
				default:
					assert(false && "unsupported device type!");
				}
			}
		}
	}

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Main functionality 
///////////////////////////////////////////////////////////////////////////////

/*
 * Handle requests until we are told to exit.
 */
static int handle_requests()
{
	int retval = 0;
	struct connection conn;

	retval = server_listen(channel, &conn);
	if(retval != SUCCESS && errno != EINTR)
		CHECK_ERR(retval);

	while(conn.msg.type != STOP_SERVER || errno == EINTR)
	{
		if(retval == SUCCESS)
		{
#ifdef _SERVER_VERBOSE
			printf("%d: ", conn.msg.sender_pid);
#endif

			switch(conn.msg.type) {
			case HW_NOTIFY:
				notify_resources(conn);
				break;
			case HW_REQUEST:
				assign_resources(conn);
				break;
			case KERNEL_FINISH:
				release_resources(conn);
				break;
			case GET_QUEUES:
				send_queues(conn);
				break;
			case CLR_QUEUES:
				clear_queues(conn);
				break;
			case HW_ASSIGN:
			case RET_QUEUES:
#ifdef _SERVER_VERBOSE
				fprintf(stderr, "client sent server-only message '%s'\n",
								message_type_str[conn.msg.type]);
#endif
				break;
			default:
#ifdef _SERVER_VERBOSE
				fprintf(stderr, "unknown message type %d\n", conn.msg.type);
#endif
				break;
			}

			numRequestsServed++;
		}

		// Get next message
		retval = server_listen(channel, &conn);
		if(retval != SUCCESS && errno != EINTR)
			CHECK_ERR(retval);
	}

	return SUCCESS;
}

/*
 * Account for client's notification by updating internal queues.
 */
static int notify_resources(struct connection& conn)
{
	Job* job = new Job(conn);

	// Must add to device's running list (cannot apply policies when daemon is
	// notified of resource usage by application).
	std::vector<size_t> indexes = utility::alloc_to_index(job->alloc);
	queues[indexes[0]]->running(job);
#ifdef _SERVER_VERBOSE
	printf("notify -> running on %lu, queues: %s\n",
		indexes[0], utility::queue_sizes().c_str());
#endif

	// Although client notified server, ack assignment
#ifdef _SERVER_STATISTICS
	numNotifies++;
#endif
	return start_job(*job);
}

/*
 * Assign hardware to client & update internal queues.
 */
static int assign_resources(struct connection& conn)
{
	bool startedRunning = false;
	Job* job = new Job(conn);
#ifdef _SERVER_STATISTICS
	struct timespec assignStart, assignEnd;
	clock_gettime(CLOCK_MONOTONIC, &assignStart);
#endif
	predictor->predict(job->features, job->predictions);

#ifdef _SERVER_VERBOSE
	printf("assign (%s) -> predictions:",
		npb_kernel_names[job->features.kernel]);
	for(float prediction : job->predictions)
		printf(" %f", prediction);
	printf(", ");
#endif

	/*
	 * 1. Check if our preferred architecture (or one within a suitable
	 *    performance threshold) is available.  If so, start running.
	 */
	// Get all candidate architectures (i.e. HW queues)
	std::vector<size_t> candidates = utility::get_candidates(job);

	// See if any candidates can begin running the job
	for(size_t candidate : candidates)
	{
		if(queues[candidate]->canRun(job))
		{
#ifdef _SERVER_VERBOSE
			printf("running on %lu", candidate);
#endif
			queues[candidate]->running(job);
			startedRunning = true;
			start_job(*job);
			break;
		}
	}

	/*
	 * 2. If no candidates can run the job, enqueue on our preferred architecture.
	 *    If there are multiple copies of our preferred architecture, choose the
	 *    one with the shortest run-queue length.
	 */
	if(!startedRunning)
	{
		size_t shortest = candidates[0];
		size_t shortestLength = queues[shortest]->numQueued();
		for(size_t i = 1; i < candidates.size(); i++)
		{
			if(!utility::same_device(shortest, i)) break;
			if(queues[candidates[i]]->numQueued() < shortestLength)
			{
				shortest = i;
				shortestLength = queues[candidates[i]]->numQueued();
			}
		}
		queues[shortest]->enqueue(job);
#ifdef _SERVER_VERBOSE
		printf("enqueued on %lu", shortest);
#endif
	}

	/* 3. Check heuristic to load balance & adjust devices */
	adjust_queues();

#ifdef _SERVER_STATISTICS
	numAssigns++;
	clock_gettime(CLOCK_MONOTONIC, &assignEnd);
	assignTime += toNS(assignEnd) - toNS(assignStart);
#endif
	return SUCCESS;
}

/*
 * After a kernel has finished, update the run queue for the appropriate
 * architecture.
 */
static int release_resources(struct connection& conn)
{
	size_t q;
	float bestPred, curPred;
	Job* job = NULL;
#ifdef _SERVER_STATISTICS
	struct timespec releaseStart, releaseEnd;
	clock_gettime(CLOCK_MONOTONIC, &releaseStart);
#endif

	/* 1. Clean up the just-finished job */
	for(q = 0; q < queues.size(); q++) {
		if(queues[q]->isRunning(conn.msg.sender_pid)) {
			job = queues[q]->finished(conn.msg.sender_pid);
			break;
		}
	}
	if(!job) return CLEANUP_ERR;

#ifdef _SERVER_VERBOSE
	printf("finished");
#ifdef _SERVER_STATISTICS
	// Because this is per-job, only print if _SERVER_VERBOSE is enabled
	printf(", queued: %lu, start: %lu, end: %lu\n",
		job->queuedTime(), job->startTime(), job->endTime());
#endif
#endif
	CHECK_ERR(server_close_connection(&conn));
	delete job;
	job = NULL;

	/* 2. Find other job to run */
	if(queues[q]->numQueued() > 0) // Start another job from same queue
		job = queues[q]->dequeue();
	else // Search for available jobs in other queues
	{
		// Note: this searches for the first job it can find that is within the
		// performance threshold.  We don't want to waste time exhaustively
		// searching for the *best* job to steal.
		for(size_t i = 0; i < queues.size(); i++) {
			for(size_t j = 0; j < queues[i]->numQueued(); j++) {
				curPred = utility::get_prediction(i, j, q);
				bestPred = utility::get_prediction(i, j, i);
				if(utility::within_threshold(bestPred, curPred))
				{
					job = queues[i]->remove(j);
					break;
				}
			}
			if(job) break;
		}
	}

	// If job is not null, we found another -- start it
	if(job)
	{
#ifdef _SERVER_VERBOSE
		printf(", %d (%s) running on %lu",
			job->client, npb_kernel_names[job->features.kernel], q);
#endif
		queues[q]->running(job);
		start_job(*job);
	}
#ifdef _SERVER_VERBOSE
	else
		printf(", %lu going idle", q);
#endif

	/* 3. Check heuristic to load balance & adjust devices */
	adjust_queues();

#ifdef _SERVER_STATISTICS
	numReleases++;
	clock_gettime(CLOCK_MONOTONIC, &releaseEnd);
	releaseTime += toNS(releaseEnd) - toNS(releaseStart);
#endif
	return SUCCESS;
}

/*
 * Send queue sizes to the client.
 */
static int send_queues(struct connection& conn)
{
#ifdef _SERVER_VERBOSE
	printf("get queues\n");
#endif

	conn.msg.sender_pid = server_pid;
	conn.msg.type = RET_QUEUES;
	size_t i;
	for(i = 0; i < queues.size(); i++)
		conn.msg.body.num_allocs[i] = queues[i]->numRunning();
	for(; i < MAX_ARCHES; i++)
		conn.msg.body.num_allocs[i] = -1;

	CHECK_ERR(server_send(&conn));
	CHECK_ERR(server_close_connection(&conn));

#ifdef _SERVER_STATISTICS
	numGetTables++;
#endif
	return SUCCESS;
}

/*
 * Clear the resource reservation table.
 */
static int clear_queues(struct connection& conn)
{
#ifdef _SERVER_VERBOSE
	printf("clear queues\n");
#endif

	for(size_t i = 0; i < queues.size(); i++)
		queues[i]->clear();

	CHECK_ERR(server_close_connection(&conn));

#ifdef _SERVER_STATISTICS
	numClears++;
#endif
	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Cleanup
///////////////////////////////////////////////////////////////////////////////

/*
 * Cleanup by deleting all files and closing all IPC channels.
 */
static int cleanup()
{
	// Check for previous cleanup
	if(cleanup_flag == 1)
		return SUCCESS;
	cleanup_flag = 1;

	// Destroy predictors + runtimes
	delete predictor;
	for(size_t i = 0; i < queues.size(); i++)
		delete queues[i];
	delete_cl_runtime(cl_rt);

	// Close & remove PID/socket file - warn if not completed correctly
	if(remove(SERVER_PID_FILE)) {
#ifdef _SERVER_VERBOSE
		perror("Error deleting PID file");
#endif
	}
	if(close_server_channel(channel)) {
#ifdef _SERVER_VERBOSE
		fprintf(stderr, "Error closing IPC channel");
#endif
	}

#ifdef _SERVER_STATISTICS
	printf("Statistics:\n"
				 "  Total number of requests: %lu\n"
				 "  Number of notifies: %lu\n"
				 "  Number of assigns: %lu\n"
				 "  Number of releases: %lu\n"
				 "  Number of get-tables: %lu\n"
				 "  Number of clears: %lu\n\n"

				 "  Average 'assign' overhead: %llu\n"
				 "  Average 'release' overhead: %llu\n",
		numRequestsServed, numNotifies, numAssigns, numReleases, numGetTables,
		numClears, assignTime / numAssigns, releaseTime / numReleases);
#endif

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Signal handlers
///////////////////////////////////////////////////////////////////////////////

/*
 * Signal handler for EXIT_SIG, sends a message that tells the server to exit.
 */
static void exit_sig(int sig)
{
	cleanup();
	exit(SUCCESS);
}

/*
 * Signal handler for CLEAR_SIG, clears the run queues in the server.
 */
static void clear_sig(int sig)
{
#ifdef _SERVER_VERBOSE
	printf("clear queues\n");
#endif

	for(size_t i = 0; i < queues.size(); i++)
		queues[i]->clear();
}

