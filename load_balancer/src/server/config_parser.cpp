#include <vector>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "server/hw_queue_config.h"
#include "server/config_parser.h"

using boost::property_tree::ptree;
using boost::property_tree::read_xml;
using boost::property_tree::write_xml;

void
ConfigParser::writeConfig(const std::vector<HWQueueConfig*>& configs,
													const std::string& configFile)
{
	ptree pt;
	std::string base("queues.");

	pt.put(base + "numQueues", configs.size());
	for(size_t i = 0; i < configs.size(); i++)
	{
		std::string cur = base + std::to_string(i) + ".";

		pt.put(cur + "platform", configs[i]->platform);
		pt.put(cur + "device", configs[i]->device);
		pt.put(cur + "computeUnits", configs[i]->computeUnits);
		pt.put(cur + "maxRunning", configs[i]->maxRunning);
		pt.put(cur + "dynamicPartitioning", configs[i]->dynamicPartitioning);
	}

	write_xml(configFile, pt);
}

void
ConfigParser::appendConfig(const std::vector<HWQueueConfig*>& configs,
													 const std::string& configFile)
{
	size_t numQueues;
	std::string base("queues.");
	ptree pt;

	read_xml(configFile, pt);
	numQueues = pt.get<size_t>(base + "numQueues");
	for(size_t i = 0; i < configs.size(); i++)
	{
		std::string cur = base + std::to_string(numQueues + i) + ".";

		pt.put(cur + "platform", configs[i]->platform);
		pt.put(cur + "device", configs[i]->device);
		pt.put(cur + "computeUnits", configs[i]->computeUnits);
		pt.put(cur + "maxRunning", configs[i]->maxRunning);
		pt.put(cur + "dynamicPartitioning", configs[i]->dynamicPartitioning);
	}

	pt.put(base + "numQueues", numQueues + configs.size());
	write_xml(configFile, pt);
}

void
ConfigParser::replaceConfig(const std::vector<std::pair<size_t, HWQueueConfig*> > configs,
														const std::string& configFile)
{
	size_t numQueues;
	std::string base("queues.");
	ptree pt;

	read_xml(configFile, pt);
	numQueues = pt.get<size_t>(base + "numQueues");

	for(std::pair<size_t, HWQueueConfig*> config : configs)
	{
		if(config.first < 0 || numQueues <= config.first)
		{
			std::cerr << "Invalid queue number: " << config.first << std::endl;
			continue; // Should we make noise?
		}

		std::string cur = base + std::to_string(config.first) + ".";
		pt.put(cur + "platform", config.second->platform);
		pt.put(cur + "device", config.second->device);
		pt.put(cur + "computeUnits", config.second->computeUnits);
		pt.put(cur + "maxRunning", config.second->maxRunning);
		pt.put(cur + "dynamicPartitioning", config.second->dynamicPartitioning);
	}

	write_xml(configFile, pt);
}

void
ConfigParser::deleteConfig(const std::vector<size_t> queueConfigs,
													 const std::string& configFile)
{
	size_t numQueues, curNum = -1;
	std::string base("queues.");
	ptree pt, newPT;

	read_xml(configFile, pt);
	numQueues = 0;

	// Find children not in queueConfigs list & add to new ptree.  This is
	// probably not the most efficient way, but it works for now.
	BOOST_FOREACH(const ptree::value_type& v, pt.get_child("queues"))
	{
		if(v.first == "numQueues")
			continue;

		if(std::find(queueConfigs.begin(),
								 queueConfigs.end(),
								 atoi(v.first.c_str())) == queueConfigs.end())
		{
			std::string cur = base + std::to_string(++curNum);
			newPT.put_child(cur, v.second);
			numQueues++;
		}
	}

	newPT.put(base + "numQueues", numQueues);
	write_xml(configFile, newPT);
}

std::vector<HWQueueConfig*>
ConfigParser::parseConfig(const std::string& configFile)
{
	size_t numQueues;
	std::string base("queues.");
	std::vector<HWQueueConfig*> queueConfigs;
	ptree pt;

	read_xml(configFile, pt);
	numQueues = pt.get<size_t>(base + "numQueues");
	for(size_t i = 0; i < numQueues; i++)
	{
		std::string cur = base + std::to_string(i) + ".";
		HWQueueConfig* config = new HWQueueConfig();

		config->platform = pt.get<size_t>(cur + "platform");
		config->device = pt.get<size_t>(cur + "device");
		config->computeUnits = pt.get<size_t>(cur + "computeUnits");
		config->maxRunning = pt.get<size_t>(cur + "maxRunning");
		config->dynamicPartitioning = pt.get<bool>(cur + "dynamicPartitioning");

		queueConfigs.push_back(config);
	}

	return queueConfigs;
}

