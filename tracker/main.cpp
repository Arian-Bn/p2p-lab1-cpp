#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>

std::set<std::string> active_peers;
std::mutex peers_mutex;

void tracker_server(int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    std::cout << "Tracker listening on port " << port << std::endl;

    while (true) {
      boost::asio::ip::tcp::socket socket(io_context);
      acceptor.accept(socket);

      // Читаем запрос от пира
      std::array<char, 256> buf;
      boost::system::error_code error;
      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (error)
        continue;

      std::string request(buf.data(), len);
      std::cout << "Tracker received: " << request << std::endl;

      std::string response;
      std::stringstream ss(request);
      std::string command, peer_addr;
      ss >> command >> peer_addr;

      {
        std::lock_guard<std::mutex> lock(peers_mutex);

        if (command == "register") {
          active_peers.insert(peer_addr);
          response = "registered";
          std::cout << "peer registered: " << peer_addr << std::endl;
        } else if (command == "unregister") {
          active_peers.erase(peer_addr);
          response = "unregistered";
          std::cout << "peer unregistered: " << peer_addr << std::endl;
        } else if (command == "get_peers") {
          for (const auto &p : active_peers) {
            if (p != peer_addr) {
              response += p + " ";
            }
          }
          if (response.empty())
            response = "none";
        }
      }

      boost::asio::write(socket, boost::asio::buffer(response));
    }
  } catch (const std::exception &e) {
    std::cerr << "Tracker error: " << e.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  int port = 9090;
  if (argc > 1)
    port = std::stoi(argv[1]);

  std::cout << "starting hybrid tracker on port " << port << std::endl;
  tracker_server(port);

  return 0;
}
