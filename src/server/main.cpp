
#include <iostream>
#include "server.hpp"

static const int PORT = 12345;
static const boost::posix_time::milliseconds TIMEOUT(5000);

int main (int argc, char* argv[])
{
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::endpoint listen_endpoint( boost::asio::ip::tcp::v4(), PORT);

	std::cout << "[Main] Creating server listening on port " << PORT << "\n";
	Server s( TIMEOUT, io_service, listen_endpoint);
	#ifdef DEBUG
	std::cout << "[Main] Starting IOSERVICE\n";
	#endif
	io_service.run();

	return 0 ;
}
