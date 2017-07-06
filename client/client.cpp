/*
 * Aplicación cliente - Colaboración con Fernando García
 * @version: 1.3p
 * @date: Junio 2017
 * @author: Daniel Perez de Andres
 *
 */



#include "client.hpp"

static const std::string PORT {"12345"};
static const std::string IP_ADDR {"localhost"};
static const int U_SEC_SLEEP = 1;
static const int SERVER_SIZE = 5;


/*
 * Constructor para clientes:
 * @param size:  número de identificadores a guardar en el servido (tamaño de la tabla en el servidor)
 * @param port:  puerto de habilitado en el servidor
 * @param ip_addr: dirección IP del servidor
 */
Client::Client(int size = SERVER_SIZE, std::string port_n = PORT, std::string ip_addr = IP_ADDR) : server_size{size} ,  port{port_n}, ip{ip_addr}
{
	// Initialize vectors with randomly assigned ids [1, 250], and names
	r_ids.reserve(server_size);
	generate_ids(r_ids);

	for (std::vector<int>::size_type i = 0 ; i < r_ids.size(); ++i ) {
		p_insert.emplace_back(std::make_pair(r_ids[i],names[i % names.size()]));
	}
}


/*
 * generate_ids
 * Crea un vector de identificadores únicos y aleatoriamente ordenados
 * @param id: vector en el que se guardan los identificadores generados
 */
void Client::generate_ids(std::vector<int>& id)
{
	for (auto i = 1 ; i <= SERVER_SIZE; ++i) {
		id.emplace_back(i);
	}
	auto engine = std::default_random_engine();

	std::shuffle(id.begin(), id.end(), engine);
	/* http://stackoverflow.com/questions/26290316/difference-between-vectorbegin-and-stdbegin
	 */
}


/*
 * connect
 * Prepara el socket y establece una conexión con el servidor
 */
Client::connect()
{
	/* Prepare socket for connection to server */
	boost::asio::io_service io_service;

	Asio::tcp::resolver resolver(io_service);

	/* If a specific server IP has been given as input, use that.
 	 * Otherwise use localhost*/
	 Asio::tcp::resolver::query query(port);

	 Asio::tcp::resolver::iterator endpoint_iterator =	resolver.resolve(query);

	 Asio::tcp::socket socket(io_service);
	 /* Connect to the server */
	 auto res = boost::asio::connect(socket, endpoint_iterator);

	 std::cout << "mete aquí un check connection" << std::endl;
	 // TODO quizas es mejor usar un oveload de connect que tiene error code como argumento y revisar la conexion con eso
	 if (res == std::end()) { // Boost asio connect return type is "end iterator" if connection fails http://charette.no-ip.com:81/programming/doxygen/boost/group__connect.html
	 			return -1;
		}
		else {
			return 0;
		}
}



Client::send()
{

	boost::asio::streambuf read_buffer;
	boost::asio::streambuf write_buffer;

	boost::system::error_code err;
	for (const auto& i : p_insert) {
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
		auto bytes_transferred = boost::asio::write(socket, write_buffer, err);
		std::cout << "element sent: bytes:" << bytes_transferred << std::endl;
		write_buffer.consume(bytes_transferred); // Remove data that was written.
		auto ts = std::chrono::high_resolution_clock::now();

		std::cout << "reading from server..." << std::endl;
		/* We only read 1 char, 0 for OK, 1 for NOK */
		bytes_transferred = boost::asio::read(socket, read_buffer,
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
			break; /* Connection closed by server */
		}
		else if (err) {
			std::cerr << "Error: " << err << std::endl;
			throw boost::system::system_error(err);
		}

		t_lapse = te - ts;
		/* http://www.cppsamples.com/common-tasks/measure-execution-time.html */

		std::cout << "I sent user: " << i.second << " to the server."<< std::endl
			<< "It replied: " <<   unsigned(resp) << std::endl << "It took " << t_lapse.count() << "ms" << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(U_SEC_SLEEP));
	}
	std::cout << "Every user sent" << std::endl;

}
