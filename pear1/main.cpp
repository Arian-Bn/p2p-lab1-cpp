#include <array>
#include <boost/asio.hpp>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

std::set<std::string> know_peers; // Список пиров
std::mutex peers_mutex;

// Функция сервера(принимает спивок от других пиров)
void server_thread(int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    std::cout << "Pear listening on port " << port << std::endl;

    while (true) {
      boost::asio::ip::tcp::socket socket(io_context);
      acceptor.accept(socket);

      // Читаем список пиров от другого пира
      std::array<char, 512> buf;
      boost::system::error_code error;
      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (!error) {
        std::string received(buf.data(), len);
        std::cout << "Received pear list: " << received << std::endl;

        std::lock_guard<std::mutex> lock(peers_mutex);
      }
    }
  }
}
