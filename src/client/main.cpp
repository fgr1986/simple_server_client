#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

/* For Sockets */
#include <iostream>

#include "client.hpp"

void client_thread(void)
{
	Client client;

	if(client.connect() == 0) { // No errors
		client.send();
	}
	else {
		std::cout << "Client in thread [" << std::this_thread::get_id() << "] could not connect to the server" << std::endl;
	}
}

int main (int argc, char* argv[])
{
	int n_clients = 5;
	std::vector<std::thread> th_pool;

	if (argc > 1) {
		std::string arg_client(argv[1]);
		n_clients = stoi(arg_client);
	}

	try {
		for (int i = 0; i < n_clients; ++i) {
			th_pool.push_back(std::thread(client_thread));
		}

		for(auto& t: th_pool) {
			t.join();
		}



	}

	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
