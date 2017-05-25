
#include <iostream>
#include <thread>
#include <sstream>
#include <algorithm>

#include "shared_resources.hpp"

char SharedResources::check_and_add_logged_clients(const std::pair<int, std::string>& recv)
{
	char res;
	#ifdef DEBUG
	std::cout << "[SharedResources]\t\tlock\n";
	#endif
	// prefer std::lock_guard to ensure the lock is released if the execution
	// throws any kind of exception
	std::lock_guard<std::mutex> g(mut_);
	/* Check if ID is in use */
	auto cli = std::find_if(logged_clients_.begin(), logged_clients_.end(),
			[&recv](const std::pair<std::pair<int, std::string>, std::string>& r){
			return recv.first == r.first.first; });
	/* In C++14 we can use (const auto& r) */
	if (cli == logged_clients_.end()) {
		auto pid = std::this_thread::get_id();
		std::stringstream ss;
		ss << pid;
		logged_clients_.emplace_back(std::make_pair(recv, ss.str()));
		#ifdef VERBOSE
		std::cout << "[SharedResources]\t\tAdded client. logged_clients_ size: " << logged_clients_.size() << "\n";
		#endif
		res = '0';
	}
	else {
		res = '1';
	}
	#ifdef DEBUG
	std::cout << "[SharedResources]\t\tunlock\n";
	#endif
	return res;
}

CInfo& SharedResources::get_logged_clients()
{
	return logged_clients_;
}
