/*
* Server App
*
* Autor: Fernando García Redondo
* fernando.garca@gmail.com
*/


#include <iostream>
#include <boost/asio.hpp>

// #include <sys/time.h>
#include <future>
// #include <thread>
#include <fstream>

// VERBOSE levels
// #define DEBUG ;
// #define VERBOSE ;
#define INFO ;

// global constants
static const std::string DELIM = ";";
static const int PORT = 12345;
boost::posix_time::milliseconds TIMEOUT(6000);

// Custom types and alias
using Uinfo = std::pair<std::pair<int, std::string>, std::string>;
using CInfo = std::vector<Uinfo>;
using Clock = std::chrono::high_resolution_clock;

class SharedResources {

public:

		char check_and_add_logged_clients(const std::pair<int, std::string>& recv)
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

		CInfo& get_logged_clients()
		{
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
			#ifdef INFO
			std::cout << "[ClientSession]\tCreating new ClientSession: " << this << ". Socketed.\n";
			#endif
			active = false;
		};

		boost::asio::ip::tcp::socket& get_socket()
		{
			return socket_;
		}

		int serve_query()
		{
			// set the session as active
			set_active();
			boost::system::error_code err;
			#ifdef INFO
			std::cout << "[ClientSession]\t\tServing query\n";
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
						std::cout << "[ClientSession]\t\tConnection closed cleanly by peer.\n";
						#endif
						break; // Connection closed cleanly by peer.
					} else if (err){
						throw boost::system::system_error(err); // Some other error.
					}

					auto data_read = make_string(read_buffer);
					#ifdef DEBUG
					std::cout << "[ClientSession]\t\tRead: " << data_read << "\n";
					#endif
					read_buffer.consume(bytes_transferred); // Remove data that was read.

					// process cli
					#ifdef DEBUG
					std::cout << "[ClientSession]\t\tProcess read information\n";
					#endif
					const std::pair<int, std::string> recv = process_cli(data_read);

					// ckeck etc
					#ifdef VERBOSE
					std::cout << "[ClientSession]\t\tCheck and add to logged clients\n";
					#endif
					char res = shared_resources_.check_and_add_logged_clients(recv);

					// Client response
					#ifdef VERBOSE
					std::cout << "[ClientSession]\t\tSend to Client the ACK\n";
					#endif
					std::ostream output(&write_buffer);
					output << res;
					bytes_transferred = boost::asio::write(socket_, write_buffer, err);
					#ifdef DEBUG
					std::cout << "[ClientSession]\t\tSent: (char) '" << res << "', bytes: " << bytes_transferred << "\n";
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
			std::cout << "[ClientSession]\tClosing thread related to serve_query. Object related to thread: " << this << "\n";
			#endif
			set_dead();

			return 0;
		}

		void close_socket()
		{
			#ifdef INFO
			std::cout << "[ClientSession]\tClose socket. Called by asio timeout\n";
			#endif
			if( socket_.is_open() ){
				boost::system::error_code ec;
				socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				socket_.close();
			}
		}

		bool is_active()
		{
			return active;
		}

		// public as it may be activated from server
		void set_active()
		{
			active = true;
		}

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

	std::string make_string(boost::asio::streambuf& streambuf)
	{
		return {boost::asio::buffers_begin(streambuf.data()),
			boost::asio::buffers_end(streambuf.data())};
	}

	std::pair<int, std::string> process_cli( std::string& content )
	{
		// remove last \n
		content = content.substr(0, content.length()-1);
		#ifdef DEBUG
		std::cout << "[ClientSession]\t\tparsing '" << content << "'\n";
		#endif
		auto locat = content.find(DELIM);
		int id = stoi(content.substr(0, locat));
		std::string name = content.substr( locat+1, content.length()-1 );
		return std::make_pair(id, name);
	}

	void set_dead()
	{
		active = false;
	}
};

class Server {

// class Server {
public:

	~Server()
	{
		#ifdef DEBUG
		std::cout << "[Server]\t\tdestructor\n";
		#endif
		// v_thread_ join
		for(auto const& t: v_thread_) {
			t->join();
		}
	};

	Server( boost::asio::io_service & io_service,
		const boost::asio::ip::tcp::endpoint & listen_endpoint )
		: io_service_(io_service),
			acceptor_(io_service, listen_endpoint)
		{
			#ifdef DEBUG
			std::cout << "[Server]\t\tconstructor\n";
			#endif
			server_exec();
	};

private:

	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::vector<std::unique_ptr<std::thread>> v_thread_;
	std::vector<std::shared_ptr<ClientSession>> client_sessions_;
	SharedResources shared_resources_;
	std::mutex mut_;

	void close_server()
	{
		#ifdef INFO
		std::cout << "[Server]\t\tExporting users to file...\n";
		#endif
		results_to_file();
		// close sockets
		#ifdef VERBOSE
		std::cout << "[Server]\t\tClosing sessions...\n";
		#endif
		close_sessions();
		acceptor_.close();
		#ifdef VERBOSE
		std::cout << "[Server]\t\tEnding execution\n";
		#endif
	}

	void close_sessions()
	{
		for(auto const& cs: client_sessions_) {
			cs->close_socket();
		}
	}

	bool are_active_sessions()
	{
		#ifdef DEBUG
		// debug, slower
		bool active_sessions = false;
		for (const auto& cs : client_sessions_) {
			if( cs->is_active() ){
				active_sessions = true;
			}
		}
		if( active_sessions ){
			std::cout << "[Server]\t\tThere are active ongoing sessions...\n";
		}else{
			std::cout << "[Server]\t\tAll sessions dead\n";
		}
		return active_sessions;
		#else
		// no debug, faster
		for (const auto& cs : client_sessions_) {
			if( cs->is_active() ){
				return true;
			}
		}
		return false;
		#endif
	}

	void add_client_session( const std::shared_ptr<ClientSession>&& cs )
	{
		#ifdef DEBUG
		std::cout << "[Server]\t\templace_back ClientSession:" << cs << "\n";
		#endif
		// prefer std::lock_guard to ensure the lock is released if the execution
		// throws any kind of exception
		std::lock_guard<std::mutex> g(mut_);
		client_sessions_.emplace_back( cs );
	}

	void timer_handler( std::shared_ptr<boost::asio::deadline_timer> timer,
		const boost::system::error_code& ec )
	{
		if(!ec) {
			if ( !are_active_sessions() ){
				#ifdef INFO
				std::cout << "[Timer]\t\tReached main timer's timeout \n";
				#endif
				close_server();
			}else{
				// still an active sesion
				#ifdef VERBOSE
				std::cout << "[Timer]\t\tReseting the timer\n";
				#endif
				timer->expires_from_now(TIMEOUT);
				timer->async_wait( std::bind(&Server::timer_handler, this, timer, ec) );
			}
		} else if( ec == boost::asio::error::operation_aborted ) {
			#ifdef INFO
			std::cout << "[Timer]\t\tOperation canceled: " << ec.message() << "\n";
			#endif
		} else {
			#ifdef INFO
			std::cout << "[Timer]\t\tTimer message: " << ec.message() << "\n";
			#endif
		}

	}

	std::shared_ptr<boost::asio::deadline_timer> get_server_timeout_timer()
	{
		#ifdef DEBUG
		std::cout << "[Server]\t\tSetting main timer: \n";
		#endif
		auto timer = std::make_shared<boost::asio::deadline_timer>( io_service_);
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
			// create the shared_ptr of ClientSession
			// Not an unique_ptr as the manager is first emplaced then the handler used for the bind
			// https://stackoverflow.com/questions/14484183/what-is-the-correct-usage-of-stdunique-ptr-while-pushing-into-stdvector
			// could be used as unique_ptr in C++17: A &element = vec.emplace_back();
			// c++11: std::shared_ptr<ClientSession> new_cs( new ClientSession( io_service_, shared_resources_) );
			// c++14
			auto cs = std::make_shared<ClientSession>( io_service_, shared_resources_ );
			#ifdef DEBUG
			std::cout << "[Server]\t\tcreated ClientSession:" << cs << "\n";
			#endif
			// add to client_sessions list
			add_client_session( std::move(cs) );
			boost::system::error_code ec;
			acceptor_.async_accept(cs->get_socket(),
					std::bind(&Server::handle_accept, this, cs, wait_connection_timer, ec ));
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << "\n";
		}
	}

	void handle_accept( std::shared_ptr<ClientSession>& cs ,
		std::shared_ptr<boost::asio::deadline_timer>& timer,
		const boost::system::error_code& ec)
	{
		#ifdef DEBUG
		std::cout << "[Server]\t\thandling ClientSession:" << cs << "\n";
		#endif
		// a client has asked us, so cancel the timer
		if( timer ){
			timer->cancel();
		}
		if (!ec) {
			// timeout, the socket is closed
			if( !cs->get_socket().is_open() ){
				#ifdef VERBOSE
				std::cout << "[Server]\t\tCalled by timeout, do not run the server_exec() again. cs: " << cs << "\n";
				#endif
				return;
			}else{
				// serve the query and run server_exec
				#ifdef VERBOSE
				std::cout << "[Server]\t\tCreating new thread related to ClientSession " << cs << "\n";
				#endif
				cs->set_active();
				std::unique_ptr<std::thread> t( new std::thread( std::bind(&ClientSession::serve_query, cs)) );
				v_thread_.emplace_back( std::move(t) );
			}
		}
		else{
			#ifdef VERBOSE
			std::cout << "[Server]\t\tError: " << ec.message() << "\n";
			#endif
		}

		#ifdef DEBUG
		std::cout << "[Server]\t\tRecursively running server_exec()\n";
		#endif
		server_exec();
	}

	void results_to_file()
	{
		std::ofstream ofs;
		try
		{
			std::string file_name ("server-"+std::to_string(getpid())+"log.log");
			ofs.open( file_name, std::ofstream::out);
			ofs << "| ID\t | Name\t | Client|\n";
			auto clients = shared_resources_.get_logged_clients();
			for (const auto& entry : clients) {
				ofs << std::to_string(entry.first.first) << "\t" << entry.first.second << "\t" << entry.second << "\n";
			}
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << "\n";
		}
		// allways close the file
		ofs.close();
	}
};

int main (int argc, char* argv[])
{
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::endpoint listen_endpoint( boost::asio::ip::tcp::v4(), PORT);

	std::cout << "\tCreating server listening on port " << PORT << "\n";
	Server s(io_service, listen_endpoint);
	#ifdef DEBUG
	std::cout << "\tStarting IOSERVICE\n";
	#endif
	io_service.run();

	return 0 ;
}
