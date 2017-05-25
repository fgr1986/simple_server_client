
#include <iostream>

#include "client_session.hpp"



ClientSession::~ClientSession(){
	#ifdef DEBUG
	std::cout << "[CSession]\t\t\tDestructor: " << this << ".\n";
	#endif
}

ClientSession::ClientSession(boost::asio::io_service& io_service, SharedResources& shared_resources)
: io_service_(io_service), socket_(io_service), shared_resources_(shared_resources)
{
	#ifdef INFO
	std::cout << "[CSession]\t\t\tCreating new ClientSession: " << this << ". Socketed.\n";
	#endif
	active = false;
}

boost::asio::ip::tcp::socket& ClientSession::get_socket()
{
	return socket_;
}

int ClientSession::serve_query()
{
	// set the session as active
	set_active();
	boost::system::error_code err;
	#ifdef INFO
	std::cout << "[CSession]\t\tServing query\n";
	#endif
	try
	{
		boost::asio::streambuf read_buffer;
		boost::asio::streambuf write_buffer;
		while (1) {
			// Read from client until new line
			auto bytes_transferred = boost::asio::read_until(socket_, read_buffer, "\n", err);
			// Read from client N bites
			// auto bytes_transferred = boost::asio::read(socket_, read_buffer,
			// 		boost::asio::transfer_exactly(bytes_transferred));
			if (err == boost::asio::error::eof){
				#ifdef INFO
				std::cout << "[CSession]\t\tConnection closed cleanly by peer.\n";
				#endif
				break; // Connection closed cleanly by peer.
			} else if (err){
				throw boost::system::system_error(err); // Some other error.
			}

			auto data_read = make_string(read_buffer);
			#ifdef DEBUG
			std::cout << "[CSession]\t\tRead: " << data_read << "\n";
			#endif
			read_buffer.consume(bytes_transferred); // Remove data that was read.

			// process cli
			#ifdef DEBUG
			std::cout << "[CSession]\t\tProcess read information\n";
			#endif
			const std::pair<int, std::string> recv = process_cli(data_read);

			// ckeck etc
			#ifdef VERBOSE
			std::cout << "[CSession]\t\tCheck and add to logged clients\n";
			#endif
			char res = shared_resources_.check_and_add_logged_clients(recv);

			// Client response
			#ifdef VERBOSE
			std::cout << "[CSession]\t\tSend to Client the ACK\n";
			#endif
			std::ostream output(&write_buffer);
			output << res;
			bytes_transferred = boost::asio::write(socket_, write_buffer, err);
			#ifdef DEBUG
			std::cout << "[CSession]\t\tSent: (char) '" << res << "', bytes: " << bytes_transferred << "\n";
			#endif
			write_buffer.consume(bytes_transferred); // Remove data that was written.
			if (err) {
				std::cerr << "Status: " << err.message() << "\n";
				return -1;
			}
		} // end of while
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
		return -1;
	}
	// set the session dead
	#ifdef INFO
	std::cout << "[CSession]\t\t\tClosing thread related to serve_query. Object related to thread: " << this << "\n";
	#endif
	set_dead();

	return 0;
}

void ClientSession::close_socket()
{
	#ifdef INFO
	std::cout << "[CSession]\t\t\tClose socket. Called by asio timeout\n";
	#endif
	if( socket_.is_open() ){
		boost::system::error_code ec;
		socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close();
	}
}

bool ClientSession::is_active()
{
	return active;
}

// public as it may be activated from server
void ClientSession::set_active()
{
	active = true;
}

std::string ClientSession::make_string(boost::asio::streambuf& streambuf)
{
	return {boost::asio::buffers_begin(streambuf.data()), boost::asio::buffers_end(streambuf.data())};
}

std::pair<int, std::string> ClientSession::process_cli( std::string& content )
{
	// remove last \n
	content = content.substr(0, content.length()-1);
	#ifdef DEBUG
	std::cout << "[CSession]\t\tparsing '" << content << "'\n";
	#endif
	auto locat = content.find(DELIM);
	int id = stoi(content.substr(0, locat));
	std::string name = content.substr( locat+1, content.length()-1 );
	return std::make_pair(id, name);
}

void ClientSession::set_dead()
{
	active = false;
}
