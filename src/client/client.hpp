#include <vector>
#include <string>
#include <chrono>

#include <boost/array.hpp>
#include <boost/asio.hpp>

using Asio = boost::asio::ip::tcp;

static const std::string PORT {"12345"};
static const std::string IP_ADDR {"localhost"};
static const int U_SEC_SLEEP = 1;
static const int SERVER_SIZE = 5;


class Client {
public:
  Client(int size = SERVER_SIZE, std::string port_n = PORT, std::string ip_addr = IP_ADDR);


  std::vector<std::pair<int, std::string>> p_insert_ {};
  int connect(void);
  int send(void);
  int receive(void);
  void pr_ids(void);


private:
  int server_size_;
  std::vector<int> r_ids_;
  std::vector<std::string> names_ {"Alice", "Bob", "Eve", "Mallory"};
  std::unique_ptr<Asio::tcp::socket> current_socket_;

  std::string port_;
  std::string ip_;
  std::chrono::duration<double, std::milli> t_lapse_;





};
