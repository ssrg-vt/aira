#ifndef _CONFIG_PARSER_H
#define _CONFIG_PARSER_H

namespace ConfigParser
{

void writeConfig(const std::vector<HWQueueConfig*>& configs,
								 const std::string& configFile);
void appendConfig(const std::vector<HWQueueConfig*>& configs,
									const std::string& configFile);
void replaceConfig(const std::vector<std::pair<size_t, HWQueueConfig*> > configs,
									 const std::string& configFile);
void deleteConfig(const std::vector<size_t> queueConfigs,
									const std::string& configFile);
std::vector<HWQueueConfig*> parseConfig(const std::string& configFile);

};

#endif /* _CONFIG_PARSER_H */
