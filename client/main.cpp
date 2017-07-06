#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

/* For Sockets */
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "client.hpp"

using Asio = boost::asio::ip::tcp;

std::string make_string(boost::asio::streambuf& streambuf)
{
	return {boost::asio::buffers_begin(streambuf.data()),
		boost::asio::buffers_end(streambuf.data())};
}




int main (int argc, char* argv[])
{
	try {


		Client client{};

    if (client.connect()) {
			client.send();
		}









	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
