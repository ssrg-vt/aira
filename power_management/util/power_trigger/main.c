#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include "power_trigger.h"

///////////////////////////////////////////////////////////////////////////////
// Configuration
///////////////////////////////////////////////////////////////////////////////

static const char* pid_file = "/var/run/power_trigger.pid";
static unsigned long pio_addr = 0xd010;
static volatile bool keep_running = true;

#define START_SIG SIGUSR1
#define STOP_SIG SIGUSR2

///////////////////////////////////////////////////////////////////////////////
// Help & parsing
///////////////////////////////////////////////////////////////////////////////

void print_help(int retval)
{
	printf("power_trigger - daemon that waits for signals to trigger (start/stop)"
	" power logging using parallel I/O\n\n"

	"Usage: ./power_trigger [ OPTIONS ]\n"
	"Options:\n"
	"  -h         : print help & exit\n"
	"  -a address : parallel port address (default is %lx)\n\n"

	"Note: must be run as root!\n\n",
		pio_addr);
	exit(retval);
}

void parse_args(int argc, char** argv)
{
	int opt;

	while((opt = getopt(argc, argv, "ha:")) != -1)
	{
		switch(opt)
		{
		case 'h':
			print_help(0);
			break;
		case 'a':
			pio_addr = strtol(optarg, NULL, 16);
			break;
		default:
			printf("Unknown argument '-%c'\n", opt);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Daemon
///////////////////////////////////////////////////////////////////////////////

void start_logging(int sig)
{
	powertrigger_begin_logging(pio_addr);
}

void stop_logging(int sig)
{
	powertrigger_end_logging(pio_addr);
}

void cleanup(int sig)
{
	keep_running = false;
}

int daemon_main()
{
	struct sigaction act;

	// Start logging signal
	act.sa_handler = start_logging;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if(sigaction(START_SIG, &act, NULL))
		exit(1);

	// Stop logging signal
	act.sa_handler = stop_logging;
	if(sigaction(STOP_SIG, &act, NULL))
		exit(1);

	// Cleanup signals
	act.sa_handler = cleanup;
	if(sigaction(SIGHUP, &act, NULL))
		exit(1);
	if(sigaction(SIGINT, &act, NULL))
		exit(1);
	if(sigaction(SIGTERM, &act, NULL))
		exit(1);

	// Write PID to file
	if(!access(pid_file, F_OK))
		exit(1);
	FILE* pidfp = fopen(pid_file, "w");
	fprintf(pidfp, "%u\n", getpid());
	fclose(pidfp);

	while(keep_running)
		pause();

	// Delete PID file
	if(remove(pid_file))
		exit(1);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Driver
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	parse_args(argc, argv);

	if(geteuid())
	{
		fprintf(stderr, "ERROR: must run daemon as root!\n");
		print_help(1);
	}

	// Fork
	pid_t pid, sid;
	pid = fork();

	if(pid < 0)
	{
		fprintf(stderr, "Could not fork child!\n");
		exit(1);
	}

	// Parent returns
	if(pid > 0)
		exit(0);

	// Set up child
	umask(0);
	sid = setsid();
	if(sid < 0)
	{
		fprintf(stderr, "Could not create process group!\n");
		exit(1);
	}

	if((chdir("/")) < 0)
	{
		fprintf(stderr, "Could not change working directory!\n");
		exit(1);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Start main loop
	return daemon_main();
}

