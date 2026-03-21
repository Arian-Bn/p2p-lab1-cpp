#include <array>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/impl/write.hpp>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
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

        // Простой парсинг (разделитель пробел)
        size_t pos = 0;
        while (pos < received.size()) {
          size_t space = received.find(' ', pos);
          if (space == std::string::npos)
            space = received.size();
          std::string peer = received.substr(pos, space - pos);
          if (!peer.empty())
            know_peers.insert(peer);
          pos = space + 1;
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
  }
}

// Функция клиента (подключается к пиру и отправляет свой список)
void connect_to_peer(const std::string &address, int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(address, std::to_string(port));

    boost::asio::connect(socket, endpoints);

    // Формируем список для отправки
    std::string peer_list;
    {
      std::lock_guard<std::mutex> lock(peers_mutex);
      for (const auto &peer : know_peers) {
        peer_list += peer + " ";
      }
    }

    boost::asio::write(socket, boost::asio::buffer(peer_list));
    std::cout << "Sent peer list to " << address << ":" << port << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Connection error: " << e.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout
        << "Usage: ./peer <port> [initial_peer_address] [initial_peer_port]"
        << std::endl;
    return 1;
  }

  int my_port = std::stoi(argv[1]);

  // Добавляем себя в список
  {
    std::lock_guard<std::mutex> lock(peers_mutex);
    know_peers.insert("127.0.0.1:" + std::to_string(my_port));
  }

  // Запускаем серверный поток
  std::thread server(server_thread, my_port);

  // Если передал начальный пир - подключаемся к нему
  if (argc == 4) {
    std::string peer_addr = argv[2];
    int peer_port = std::stoi(argv[3]);
    connect_to_peer(peer_addr, peer_port);
  }

  // Периодически рассылаем свой список всем известным пирам
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::set<std::string> current_peers;
    {
      std::lock_guard<std::mutex> lock(peers_mutex);
      current_peers = know_peers;
    }

    for (const auto &peer : current_peers) {
      if (peer == "127.0.0.1:" + std::to_string(my_port))
        continue;

      size_t colon = peer.find(':');
      std::string addr = peer.substr(0, colon);
      int port = std::stoi(peer.substr(colon + 1));
      connect_to_peer(addr, port);
    }
  }

  server.join();
  return 0;
}
