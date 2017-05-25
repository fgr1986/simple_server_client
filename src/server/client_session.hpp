#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <boost/asio.hpp>

#include "shared_resources.hpp"

class ClientSession {
	public:
		
		~ClientSession();
		ClientSession(boost::asio::io_service& io_service, SharedResources& shared_resources);
		boost::asio::ip::tcp::socket& get_socket();
		int serve_query();
		void close_socket();
		bool is_active();

		// public as it may be activated from server
		void set_active();

private:
	friend class SharedResources;
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;
	SharedResources& shared_resources_;
	// Option 1: mutex
	// use it with
	// std::lock_guard<std::mutex> g(mut_);
	// std::mutex mut_;
	// bool active;

	// Option 2: atomic bool
	// more eficient for ints, bools etc
	std::atomic<bool> active;

	// functions
	std::string make_string(boost::asio::streambuf& streambuf);
	std::pair<int, std::string> process_cli( std::string& content );
	void set_dead();

};

#endif /* CLIENT_SESSION_H */
