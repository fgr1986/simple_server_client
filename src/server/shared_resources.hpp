#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

// VERBOSE levels
#define DEBUG ;
#define VERBOSE ;
#define INFO ;

#include <vector>
#include <mutex>

// global constants
static const std::string DELIM = ";";

// Custom types and alias
using Uinfo = std::pair<std::pair<int, std::string>, std::string>;
using CInfo = std::vector<Uinfo>;

class SharedResources {

public:

		char check_and_add_logged_clients(const std::pair<int, std::string>& recv);
		CInfo& get_logged_clients();

private:

	std::mutex mut_logged_clients_;
	CInfo logged_clients_;

};

#endif /* SHARED_RESOURCES_H */
