/*
* Server App
*
* Autor: Fernando García Redondo
* fernando.garca@gmail.com
*/


#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <thread>

// #include <future>
// #include <thread>

#include "shared_resources.hpp"
#include "client_session.hpp"

class Server {

// class Server {
public:

	~Server();
	Server( const boost::posix_time::milliseconds timeout,
		boost::asio::io_service & io_service,
		const boost::asio::ip::tcp::endpoint & listen_endpoint );

private:

	const boost::posix_time::milliseconds timeout_;

	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::vector<std::unique_ptr<std::thread>> v_thread_;
	std::vector<std::shared_ptr<ClientSession>> client_sessions_;
	SharedResources shared_resources_;
	std::mutex mut_;

	// Server methods
	void close_server();
	void close_sessions();
	bool are_active_sessions();
	void add_client_session( const std::shared_ptr<ClientSession>&& cs );

	void restart_handler( const std::shared_ptr<boost::asio::deadline_timer>&& timer );
	void timer_handler( const std::shared_ptr<boost::asio::deadline_timer>&& timer,
		const boost::system::error_code&& ec );
	std::shared_ptr<boost::asio::deadline_timer> get_server_timeout_timer();
	void server_exec (void);
	void handle_session( const std::shared_ptr<ClientSession>&& cs ,
		const std::shared_ptr<boost::asio::deadline_timer>&& timer,
		const boost::system::error_code& ec);
	void results_to_file();

};

#endif /* SERVER_H */
