/*
* Aplicación servidor- Proceso de Selección By tech
* ref: S158p1-2
* v: 1.3p
* Supuesto 2
*
* Autor: Daniel Pérez de Andrés
*/


#include <iostream>
#include <boost/asio.hpp>
// #include <boost/enable_shared_from_this.hpp>

#include <sys/time.h>
#include <future>
#include <thread>
#include <fstream>

// global constant
static const std::string DELIM = ";";
static const int PORT = 12345;

boost::posix_time::milliseconds TIMEOUT(6000);

using Uinfo = std::pair<std::pair<int, std::string>, std::string>;
using CInfo = std::vector<Uinfo>;
using Clock = std::chrono::high_resolution_clock;

class SharedResources {

public:

		char check_and_add_logged_clients(const std::pair<int, std::string>& recv)
		{
			int res;
			std::lock_guard<std::mutex> g(mut_);
			std::cout << "[ClientSession]\t\tlock\n";
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
				std::cout << "[ClientSession]\t\tlogged_clients_ size: " << logged_clients_.size() << std::endl;
				res = '0';
			}
			else {
				res = '1';
			}
			std::cout << "[ClientSession]\t\tunlock\n";
			return res;
		}

		CInfo& get_logged_clients(){
			return logged_clients_;
		}

private:

	std::mutex mut_;
	CInfo logged_clients_;

};

class ClientSession {
	public:

		ClientSession(boost::asio::io_service& io_service, SharedResources& shared_resources)
		: io_service_(io_service), socket_(io_service), shared_resources_(shared_resources)
		{
			std::cout << "[ClientSession]\tCreating new ClientSession. Socketed." << std::endl;
			active = false;
		};

		boost::asio::ip::tcp::socket& get_socket(){
			return socket_;
		}

		int serve_query()
		{
			// set the session as active
			set_active();
			boost::system::error_code err;
			std::cout << "[ClientSession]\t\tServing query" << std::endl;
			try
			{
				boost::asio::streambuf read_buffer;
				boost::asio::streambuf write_buffer;
				while (1) {
					// Read from client.
					auto bytes_transferred = boost::asio::read_until(socket_, read_buffer, "\n", err);

					if (err == boost::asio::error::eof){
						std::cout << "[ClientSession]\t\tConnection closed cleanly by peer." << std::endl;
						break; // Connection closed cleanly by peer.
					} else if (err){
						throw boost::system::system_error(err); // Some other error.
					}

					// auto bytes_transferred = boost::asio::read(socket_, read_buffer,
					// 		boost::asio::transfer_exactly(bytes_transferred));
					auto data_read = make_string(read_buffer);
					std::cout << "[ClientSession]\t\tRead: " << data_read << std::endl;
					read_buffer.consume(bytes_transferred); // Remove data that was read.

					// process cli
					std::cout << "[ClientSession]\t\tprocess_cli" << std::endl;
					const std::pair<int, std::string> recv = process_cli(data_read);

					// ckeck etc
					std::cout << "[ClientSession]\t\tcheck_and_add_logged_clients" << std::endl;
					char res = shared_resources_.check_and_add_logged_clients(recv);

					// Responder
					std::cout << "[ClientSession]\t\tresponder al cliente" << std::endl;
					std::ostream output(&write_buffer);
					output << res;
					bytes_transferred = boost::asio::write(socket_, write_buffer, err);
					std::cout << "[ClientSession]\t\tSent: (char) '" << res << "', bytes: " << bytes_transferred <<	std::endl;
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
			set_dead();
			std::cout << "[ClientSession]\tClosing thread related to serve_query" << std::endl;
			std::cout <<"[debug] closing cs:" << this << std::endl;
			return 0;
		}

		void close_socket()
		{
			if( socket_.is_open() ){
				std::cout << "[ClientSession]\tClose socket by asio timeout!!!" << std::endl;
				boost::system::error_code ec;
				socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				socket_.close();
			}
		}

		bool is_active(){
			std::lock_guard<std::mutex> g(mut_);
			return active;
		}


		void set_active(){
			std::lock_guard<std::mutex> g(mut_);
			active = true;
			std::cout << "[debug] \tSesion set as active" << std::endl;
		}

private:
	friend class SharedResources;
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;
	SharedResources& shared_resources_;
	std::mutex mut_;
	bool active;

	std::string make_string(boost::asio::streambuf& streambuf)
	{
		return {boost::asio::buffers_begin(streambuf.data()),
			boost::asio::buffers_end(streambuf.data())};
	}

	std::pair<int, std::string> process_cli( std::string& content )
	{
		// remove last \n
		content = content.substr(0, content.length()-1);
		std::cout << "[ClientSession]\t\tparsing '" << content << "'\n";
		auto locat = content.find(DELIM);
		int id = stoi(content.substr(0, locat));
		std::string name = content.substr( locat+1, content.length()-1 );
		return std::make_pair(id, name);
	}

	void set_dead(){
		std::lock_guard<std::mutex> g(mut_);
		active = false;
		std::cout << "[debug] \tSesion set as inactive" << std::endl;
	}
};

class Server {

// class Server {
public:

	~Server(){
		std::cout << "\tServer destructor" << std::endl;
		// v_thread_ join?

		for(auto const& t: v_thread_) {
			t->join();
		}
	};

	Server( boost::asio::io_service & io_service,
		const boost::asio::ip::tcp::endpoint & listen_endpoint )
		: io_service_(io_service),
			acceptor_(io_service, listen_endpoint)
			// timer_(io_service)
		{
			std::cout << "\tServer constructor" << std::endl;
			server_exec();
	};

private:

	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::vector<std::shared_ptr<std::thread>> v_thread_;
	std::vector<std::shared_ptr<ClientSession>> client_sessions_;
	SharedResources shared_resources_;

	void close_server(){
		std::cout << "\tExporting users to file..." << std::endl;
		results_to_file();
		// close sockets
		close_sessions();
		acceptor_.close();
		std::cout << "\tEnding execution..." << std::endl;
	}

	void close_sessions(){
		for(auto const& cs: client_sessions_) {
			cs->close_socket();
		}
	}

	bool are_active_sessions(){
		// std::lock_guard<std::mutex> g(mut_);
		bool active_sessions = false;
		for (const auto& cs : client_sessions_) {
			std::cout <<"[debug] checking session:" << cs << std::endl;
			if( cs->is_active() ){
				std::cout <<"[debug] session active:" << cs << std::endl;
				active_sessions = true;
			}
		}
		if(active_sessions){
			std::cout << "[debug]\tactive_sessions active" << std::endl;
		}else{
			std::cout << "[debug]\tactive_sessions dead" << std::endl;
		}
		return active_sessions;
	}

	void timer_handler( std::shared_ptr<boost::asio::deadline_timer> timer,
		const boost::system::error_code& ec ){

		if(!ec)
		{
			if ( !are_active_sessions() ){
				std::cout << "[Timer] Reached main timer's timeout " << std::endl;
				boost::system::error_code ec;
				close_server();
			}else{
				// still an active sesion
				std::cout << "[Timer] Reseting the timer" << std::endl;
				timer->expires_from_now(TIMEOUT);
				timer->async_wait( std::bind(&Server::timer_handler, this, timer, ec) );
			}
		}else if( ec == boost::asio::error::operation_aborted )
			std::cout << "[Timer] Operation canceled: " << ec.message() << std::endl;
		else
			std::cout << "[Timer] ec message: " << ec.message() << std::endl;

	}

	std::shared_ptr<boost::asio::deadline_timer> get_server_timeout_timer()
	{
		std::cout << "[Timer] Setting main timer: "  << std::endl;
		std::shared_ptr<boost::asio::deadline_timer> timer( new  boost::asio::deadline_timer(io_service_));
		timer->expires_from_now(TIMEOUT);
		//asign lambda
		boost::system::error_code ec;
		timer->async_wait( std::bind(&Server::timer_handler, this, timer, ec) );
		return timer;
	}

	// Server methods
	void server_exec (void)
	{
		try
		{
			auto wait_connection_timer = get_server_timeout_timer();
			// create the session
			std::shared_ptr<ClientSession> new_cs( new ClientSession( io_service_, shared_resources_) );
			std::cout <<"[debug] created cs:" << new_cs << std::endl;
			client_sessions_.emplace_back( new_cs );
			boost::system::error_code ec;
			acceptor_.async_accept(new_cs->get_socket(),
					std::bind(&Server::handle_accept, this, new_cs, wait_connection_timer, ec ));
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}

	void handle_accept(std::shared_ptr<ClientSession> cs , std::shared_ptr<boost::asio::deadline_timer> timer,
		const boost::system::error_code& ec)
	{

		std::cout <<"[debug] handling cs:" << cs << std::endl;
		// a client has asked us, so cancel the timer
		if( timer ){
			timer->cancel();
		}
		if (!ec)
		{
			// timeout, the socket is closed
			if( !cs->get_socket().is_open() ){
				std::cout << "\tCalled by timeout, do not run the server_exec() again. cs: " << cs << std::endl;
				return;
			}else{
				// serve the query and run server_exec
				// cs->serve_query();
				// detached thread
				std::cout << "[debug]\tCreating new thread" << std::endl;
				cs->set_active();
				std::shared_ptr<std::thread> t( new std::thread( std::bind(&ClientSession::serve_query, cs)) );
				v_thread_.emplace_back( t );
				// the thread will be destroyed by unique_ptr
				// std::unique_ptr<std::thread> t( new std::thread(
				// 	std::bind(&ClientSession::serve_query, std::move(cs))) );
				// t->detach();
			}
		}
		else
			std::cout << ec.message() << std::endl;

		std::cout << "\tRecursively running server_exec()" << std::endl;
		server_exec();
	}

	void results_to_file()
	{
		std::ofstream ofs;
		try
		{
			std::string file_name ("server-"+std::to_string(getpid())+"log.log");
			ofs.open( file_name, std::ofstream::out);
			ofs << "| ID\t | Name\t | Client|" << std::endl;
			auto clients = shared_resources_.get_logged_clients();
			for (const auto& entry : clients) {
				ofs << std::to_string(entry.first.first) << "\t" << entry.first.second << "\t" << entry.second << std::endl;
			}
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
		// allways close the file
		ofs.close();
	}
};

int main (int argc, char* argv[])
{
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::endpoint listen_endpoint( boost::asio::ip::tcp::v4(), PORT);

	std::cout << "\tCreating server listening on port " << PORT << std::endl;
	Server s(io_service, listen_endpoint);
	std::cout << "\tStarting IOSERVICE" << std::endl;
	io_service.run();

	return 0 ;
}
