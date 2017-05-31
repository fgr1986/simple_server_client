/*
* Server App
*
* Autor: Fernando García Redondo
* fernando.garca@gmail.com
*/


#include <iostream>
#include <fstream>

#include "server.hpp"

Server::~Server()
{
	#ifdef DEBUG
	std::cout << "[Server]\tdestructor\n";
	#endif
	// v_thread_ join
	for(auto const& t: v_thread_) {
		t->join();
	}
}

Server::Server( const boost::posix_time::milliseconds timeout,
	boost::asio::io_service & io_service,
	const boost::asio::ip::tcp::endpoint & listen_endpoint )
	: timeout_(timeout),
		io_service_(io_service),
		acceptor_(io_service, listen_endpoint)
{
		#ifdef DEBUG
		std::cout << "[Server]\tconstructor: " << this << "\n";
		#endif
		server_exec();
}


void Server::close_server()
{
	#ifdef INFO
	std::cout << "[Server]\tExporting users to file...\n";
	#endif
	results_to_file();
	#ifdef VERBOSE
	std::cout << "[Server]\tClosing sessions...\n";
	#endif
	// close acceptor
	acceptor_.close();
	// close sockets
	close_sessions();
	#ifdef VERBOSE
	std::cout << "[Server]\tEnding execution\n";
	#endif
}

void Server::close_sessions()
{
	for(auto const& cs: client_sessions_) {
		cs->close_socket();
	}
}

bool Server::are_active_sessions()
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
		std::cout << "[Server]\tThere are active ongoing sessions...\n";
	}else{
		std::cout << "[Server]\tAll sessions dead\n";
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

void Server::add_client_session( const std::shared_ptr<ClientSession>&& cs )
{
	#ifdef DEBUG
	std::cout << "[Server]\templace_back ClientSession:" << cs << "\n";
	#endif
	// prefer std::lock_guard to ensure the lock is released if the execution
	// throws any kind of exception
	std::lock_guard<std::mutex> g(mut_);
	client_sessions_.emplace_back( cs );
}
void Server::restart_handler( const std::shared_ptr<boost::asio::deadline_timer>&& timer )
{
	#ifdef DEBUG
	std::cout << "[Timer]\t\t\t--> [A] Restarting the timer.\n";
	#endif
	timer->cancel();
	timer->expires_from_now(timeout_);
	// timer->async_wait( std::bind(&Server::timer_handler, this, timer, ec) );
	timer->async_wait(
		[this, timer](boost::system::error_code ec){
			timer_handler( std::move(timer), std::move(ec) );
	} );
	#ifdef DEBUG
	std::cout << "[Timer]\t\t\t--> [B] Restarting the timer.\n";
	#endif
}

void Server::timer_handler( const std::shared_ptr<boost::asio::deadline_timer>&& timer,
	const boost::system::error_code&& ec )
{
	#ifdef DEBUG
	std::cout << "[Timer]\t\t\t--> Timer_handler: " << ".\n";
	#endif
	if( !ec ) {
		if ( !are_active_sessions() ){
			#ifdef INFO
			std::cout << "[Timer]\t\t\t--> Reached main timer's timeout \n";
			#endif
			close_server();
		} else {
			// still an active sesion
			restart_handler( std::move(timer) );
		}
	} else if( ec == boost::asio::error::operation_aborted ) {
		#ifdef INFO
		std::cout << "[Timer]\t\t\t--> Operation canceled: " << ec.message() << "\n";
		#endif
	} else {
		#ifdef INFO
		std::cout << "[Timer]\t\t\t--> Timer message: " << ec.message() << "\n";
		#endif
	}

}

std::shared_ptr<boost::asio::deadline_timer> Server::get_server_timeout_timer()
{
	#ifdef DEBUG
	std::cout << "[Server]\tSetting main timer: \n";
	#endif
	auto timer = std::make_shared<boost::asio::deadline_timer>( io_service_);
	restart_handler( std::move(timer) );
	return timer;
}

void Server::server_exec()
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
		std::cout << "[Server]\tcreated ClientSession:" << cs << "\n";
		#endif
		// add to client_sessions list
		add_client_session( std::move(cs) );

		// old style
		// boost::system::error_code ec;
		// acceptor_.async_accept(cs->get_socket(),
		// 		std::bind(&Server::handle_session, this, cs, wait_connection_timer, ec ));

		// modern style
		// lambda capture this, and smart pointers by ref
		// cs by ref? no, by copy so the pointer destructor is not called
		// then moved
		acceptor_.async_accept(cs->get_socket(),
			[this, cs, wait_connection_timer](boost::system::error_code ec){
				handle_session( std::move(cs), std::move(wait_connection_timer), ec );
		});
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

void Server::handle_session( const std::shared_ptr<ClientSession>&& cs ,
	const std::shared_ptr<boost::asio::deadline_timer>&& timer,
	const boost::system::error_code& ec)
{
	if( cs==nullptr ){
		throw std::runtime_error("[Server]\tNull ClientSession");
	}
	// if the server closes the sessions (and the sockets)
	// the handler is called back
	if( ec==boost::asio::error::operation_aborted ){
		#ifdef VERBOSE
		std::cout << "[Server]\tEnding sleeping ClientSession" << cs << "\n";
		#endif
		return;
	}
	// a client has asked us, so cancel the timer
	if( timer ){
		#ifdef VERBOSE
		std::cout << "[Server]\tA client has asked us, so cancel the timer" << "\n";
		#endif
		restart_handler( std::move(timer) );
	}
	if (!ec) {
		// timeout, the socket is closed
		if( !cs->get_socket().is_open() ){
			#ifdef VERBOSE
			std::cout << "[Server]\tCalled by timeout, do not run the server_exec() again. cs: " << cs << "\n";
			#endif
			return;
		}else{
			// serve the query and run server_exec
			#ifdef VERBOSE
			std::cout << "[Server]\tCreating new thread related to ClientSession " << cs << "\n";
			#endif
			cs->set_active();
			// cs captured by value as it is an unique pointer and the movement
			// would destroy the previous handler
			std::unique_ptr<std::thread> t( new std::thread(
				[cs](){ cs->serve_query(); }
			));

			v_thread_.emplace_back( std::move(t) );
		}
	} else {
		#ifdef VERBOSE
		std::cout << "[Server]\tError: " << ec.message() << "\n";
		#endif
		return;
	}

	#ifdef DEBUG
	std::cout << "[Server]\tRecursively running server_exec()\n";
	#endif
	server_exec();
}

void Server::results_to_file()
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
