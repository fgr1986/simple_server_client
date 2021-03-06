/*
 * Aplicación cliente - Colaboración con Fernando García
 * @version: 1.3p
 * @date: Junio 2017
 * @author: Daniel Perez de Andres
 *
 */



#include "client.hpp"
#include <iostream>
#include <random>
#include <algorithm>
#include <thread>


using Asio = boost::asio::ip::tcp;

std::string make_string(boost::asio::streambuf& streambuf)
{
	return {boost::asio::buffers_begin(streambuf.data()),
		boost::asio::buffers_end(streambuf.data())};
}

template <typename T>
constexpr bool is_lvalue(T&&) noexcept
{
	  return std::is_lvalue_reference<T>{};
}

/*
 * Constructor para clientes:
 * @param size:  número de identificadores a guardar en el servido (tamaño de la tabla en el servidor)
 * @param port:  puerto de habilitado en el servidor
 * @param ip_addr: dirección IP del servidor
 */
Client::Client(int size, std::string port_n, std::string ip_addr)
	: server_size_{size} ,  port_{port_n}, ip_{ip_addr}
{
	// Initialize vectors with randomly assigned ids [1, size], and names
	std::vector<int> r_ids(server_size_);
	r_ids.reserve(server_size_); // not needed
	// fill it
	int n = 0;
	std::generate(r_ids.begin(), r_ids.end(), [&n]() { return n++; });
	// suffle it
	auto engine = std::default_random_engine();
	std::shuffle(r_ids.begin(), r_ids.end(), engine);
	// generate pair
	n = 0;
	for ( const auto & id : r_ids ) {
		p_insert_.emplace_back( std::make_pair(id, names_[n++ % names_.size()]));
	}
}

/*
 * connect
 * Prepara el socket y establece una conexión con el servidor
 */
int Client::connect(void)
{
	// Prepare socket for connection to server
	boost::asio::io_service io_service;
	Asio::tcp::resolver resolver(io_service);

	// If a specific server IP has been given as input, use that.
	// Otherwise use localhost
	Asio::tcp::resolver::query query(port_);
	Asio::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	current_socket_.reset(new Asio::tcp::socket(io_service));

	// Connect to the server
	boost::system::error_code error;
	boost::asio::connect(*current_socket_, endpoint_iterator, error);
	// check for errors in connection
	if( error ){
		std::cerr << "An Error occurred when connecting to the server: '"
				<< error.message() << "'" << std::endl;
		return -1;
	}
	return 0;
}



int Client::send(void)
{
	if (current_socket_ == nullptr) {
		std::cerr << "current_socket_ == nullptr. Invalid socket" <<
			std::endl;
		return -1;
	}
	int res = 0;

	boost::asio::streambuf read_buffer;
	boost::asio::streambuf write_buffer;

	boost::system::error_code err;
	for (const auto& i : p_insert_) {
		std::cout << "sending element: " << std::to_string(i.first) << std::endl;

		/* Write to server. */
		std::ostream output(&write_buffer);
		output << std::to_string(i.first) + ";" + i.second + "\n";

		/* alternative option:
		 * define messages:
		 * +----------------+-------------------------------------------+
		 * |	Length				|	Message string	|
		 * |				4 bytes |		n bytes |
		 * +----------------+-------------------------------------------+
		 */
		std::cout << "Writing: " << make_string(write_buffer) << std::endl;
		auto bytes_transferred = boost::asio::write(*current_socket_, write_buffer, err);
		std::cout << "element sent: bytes:" << bytes_transferred << std::endl;
		write_buffer.consume(bytes_transferred); // Remove data that was written.
		auto ts = std::chrono::high_resolution_clock::now();

		std::cout << "reading from server..." << std::endl;
		/* We only read 1 char, 0 for OK, 1 for NOK */
		bytes_transferred = boost::asio::read(*current_socket_, read_buffer,
				boost::asio::transfer_exactly(sizeof(char)));

		/* read to int */
		std::istream is(&read_buffer);
		/* usual extraction: */
		uint resp;
		if (is >> resp) {
			std::cout << "Read: " << unsigned(resp) << std::endl;
			std::cout << "Size of read_buffer: " << bytes_transferred << std::endl;
		}else{
			std::cout << "Error reading server response: " << resp << std::endl;
			throw boost::system::system_error(err);
		}
		read_buffer.consume(bytes_transferred); /* Remove data read from buffer */

		auto te = std::chrono::high_resolution_clock::now();

		if (err == boost::asio::error::eof) {
			std::cout << "Connection closed by server: " << resp << std::endl;
			res = -1;
			break; /* Connection closed by server */
		}
		else if (err) {
			std::cerr << "Error: " << err << std::endl;
			throw boost::system::system_error(err);
			res = -1;
		}

		t_lapse_ = te - ts;
		/* http://www.cppsamples.com/common-tasks/measure-execution-time.html */

		std::cout << "I sent user: " << i.second << " to the server."<< std::endl
			<< "It replied: " <<   unsigned(resp) << std::endl << "It took " << t_lapse_.count() << "ms" << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(U_SEC_SLEEP));
	}
	std::cout << "Every user sent" << std::endl;

	return res;

}
