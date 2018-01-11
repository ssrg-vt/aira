#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifndef _PERSISTENT_H
#define _PERSISTENT_H

/*
 * Interface requiring child classes to implement functions that can save and
 * restore objects from disk.
 */
class Persistent
{
public:
	virtual int save(std::string& filename) = 0;
	virtual int load(std::string& filename) = 0;
};

#endif /* _PERSISTENT_H */
