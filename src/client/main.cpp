#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

/* For Sockets */
#include <iostream>

#include "client.hpp"



int main (int argc, char* argv[])
{
	try {
		Client client;

		if (client.connect()) {
			client.send();
		}

	}

	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
