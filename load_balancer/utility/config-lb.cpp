#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#include "server/hw_queue_config.h"
#include "server/config_parser.h"

static const char* help =
"config-lb - query and modify hardware queue configuration information for "
"aira-lb.  See the OpenCL runtime for information on platforms, devices & "
"compute units available.\n\n"

"Usage: ./config-lb <action> [ OPTIONS ]\n\n"

"Actions:\n"
"  print   : print the current configuration in the configuration file\n"
"  write   : write a command-line specified configuration to the configuration file\n"
"  append  : append a command-line specified configuration to an existing configuration file\n"
"  replace : replace one of the configurations with another\n"
"  delete  : delete a configuration\n\n"

"Options (all actions):\n"
"  -h      : print help & exit\n"
"  -c file : configuration file to to be printed/written\n\n"

"Options (write, append):\n"
"  -d #,#,#,#,bool : N-tuple specifying platform, device, number of compute units, max number of\n"
"                    concurrently running applications, and a boolean specifying if dynamic\n"
"                    partitioning is available.  Can be specified multiple times.\n\n"

"Options (replace):\n"
"  -r #,#,#,#,#,bool : N-tuple specifying queue configuration to replace, platform, device, number\n"
"                      of compute units, max number of concurrently running applications, and a\n"
"                      boolean specifying if dynamic partitioning is available.  Can be specified\n"
"                      multiple times.\n"

"Options (delete):\n"
"  -e # : Which HW-queue configuration to delete.  Can be specified multiple times.\n";

static std::string config_fn = "config.xml";
static std::vector<HWQueueConfig*> toWrite;
static std::vector<std::pair<size_t, HWQueueConfig*> > toReplace;
static std::vector<size_t> toErase;

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

void printConfig()
{
	std::vector<HWQueueConfig*> configs = ConfigParser::parseConfig(config_fn);

	if(configs.size() == 0)
		std::cout << "No configurations in file '" << config_fn
							<< "'" << std::endl;
	else
	{
		std::cout << std::setw(8) << std::left << "Config"
							<< std::setw(10) << "Platform"
							<< std::setw(8) << "Device"
							<< std::setw(6) << "CUs"
							<< std::setw(17) << "Max Concurrency"
							<< std::setw(22) << "Dynamic Partitioning"
							<< std::endl;
		for(size_t i = 0; i < configs.size(); i++)
			std::cout << std::setw(8) << i
								<< std::setw(10) << configs[i]->platform
								<< std::setw(8) << configs[i]->device
								<< std::setw(6) << configs[i]->computeUnits
								<< std::setw(17) << configs[i]->maxRunning
								<< std::setw(22) << (configs[i]->dynamicPartitioning ? "enabled" : "disabled")
								<< std::endl;
	}
}

void writeConfig()
{
	ConfigParser::writeConfig(toWrite, config_fn);
}

void appendConfig()
{
	ConfigParser::appendConfig(toWrite, config_fn);
}

void replaceConfig()
{
	ConfigParser::replaceConfig(toReplace, config_fn);
}

void deleteConfig()
{
	ConfigParser::deleteConfig(toErase, config_fn);
}

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

HWQueueConfig* parseConfig(const char* tuple)
{
	std::string input(tuple);
	std::stringstream ss(input);
	std::string token;
	HWQueueConfig* config = new HWQueueConfig();

	std::getline(ss, token, ',');
	config->platform = atoi(token.c_str());
	std::getline(ss, token, ',');
	config->device = atoi(token.c_str());
	std::getline(ss, token, ',');
	config->computeUnits = atoi(token.c_str());
	std::getline(ss, token, ',');
	config->maxRunning = atoi(token.c_str());
	std::getline(ss, token, ',');
	config->dynamicPartitioning = (token == "true" ? true : false);

	return config;
}

std::pair<size_t, HWQueueConfig*> parseReplacementConfig(const char* tuple)
{
	std::string input(tuple);
	std::stringstream ss(input);
	std::string token;
	std::pair<size_t, HWQueueConfig*> pair;
	pair.second = new HWQueueConfig();

	std::getline(ss, token, ',');
	pair.first = atoi(token.c_str());
	std::getline(ss, token, ',');
	pair.second->platform = atoi(token.c_str());
	std::getline(ss, token, ',');
	pair.second->device = atoi(token.c_str());
	std::getline(ss, token, ',');
	pair.second->computeUnits = atoi(token.c_str());
	std::getline(ss, token, ',');
	pair.second->maxRunning = atoi(token.c_str());
	std::getline(ss, token, ',');
	pair.second->dynamicPartitioning = (token == "true" ? true : false);

	return pair;
}

///////////////////////////////////////////////////////////////////////////////
// Driver
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	int c;
	const char* action;

	if(argc < 2)
	{
		std::cerr << "Not enough arguments!" << std::endl << std::endl << help;
		exit(1);
	}

	action = argv[1];
	if(!strcmp(action, "help") || !strcmp(action, "-h") || !strcmp(action, "--help"))
	{
		std::cout << help;
		exit(0);
	}

	argc -= 1;
	argv += 1;

	while((c = getopt(argc, argv, "hc:d:r:e:")) != -1)
	{
		switch(c)
		{
		case 'h':
			std::cout << help;
			exit(0);
			break;
		case 'c':
			config_fn = optarg;
			break;
		case 'd':
			toWrite.push_back(parseConfig(optarg));
			break;
		case 'r':
			toReplace.push_back(parseReplacementConfig(optarg));
			break;
		case 'e':
			toErase.push_back(atoi(optarg));
			break;
		default:
			std::cerr << "Warning: unknown argument '" << (char)c << "'" << std::endl;
			break;
		}
	}

	if(!strcmp(action, "print"))
		printConfig();
	if(!strcmp(action, "write"))
		writeConfig();
	if(!strcmp(action, "append"))
		appendConfig();
	if(!strcmp(action, "replace"))
		replaceConfig();
	if(!strcmp(action, "delete"))
		deleteConfig();

	return 0;
}

