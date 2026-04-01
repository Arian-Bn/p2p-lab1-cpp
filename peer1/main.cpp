#include <array>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>

std::set<std::string> know_peers; // Список пиров
std::mutex peers_mutex;
std::string tracker_host = "127.0.0.1";
int tracker_port = 9090;

// Функция для отправки запросов к трекеру
std::string send_to_tracker(const std::string &request) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoint =
        resolver.resolve(tracker_host, std::to_string(tracker_port));
    boost::asio::connect(socket, endpoint);
    boost::asio::write(socket, boost::asio::buffer(request));

    std::array<char, 1024> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);

    if (!error) {
      return std::string(buf.data(), len);
    } else {
      return "Error tracker response!";
    }
  } catch (const std::exception &e) {
    std::cerr << "Tracker communication error: " << e.what() << std::endl;
    return "";
  }
}

// Функция сервера (принимает от других пиров)
void server_thread(int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    std::cout << "Peer listening on port " << port << std::endl;

    while (true) {
      boost::asio::ip::tcp::socket socket(io_context);
      acceptor.accept(socket);

      std::array<char, 512> buf;
      boost::system::error_code error;
      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (!error) {
        std::string received(buf.data(), len);
        std::cout << "Received peer list: " << received << std::endl;

        std::lock_guard<std::mutex> lock(peers_mutex);
        std::stringstream ss(received);
        std::string peer;
        while (ss >> peer) {
          know_peers.insert(peer);
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
  }
}

// Функция клиента (подключается к пиру и отправляет свой список)
void connect_to_peer(const std::string &address, int port,
                     const std::string &my_addr) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(address, std::to_string(port));
    boost::asio::connect(socket, endpoints);

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
    std::cerr << "Connect error to " << address << ":" << port << " - "
              << e.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage ./peer1 <port>" << std::endl;
    return 1;
  }

  int my_port = std::stoi(argv[1]);
  std::string my_addr = "127.0.0.1:" + std::to_string(my_port);

  // Регистрируемся в трекере
  std::cout << "Registering with tracker..." << std::endl;
  std::string response = send_to_tracker("register " + my_addr);
  std::cout << "Tracker response: " << response << std::endl;

  // Добавляем себя в список
  {
    std::lock_guard<std::mutex> lock(peers_mutex);
    know_peers.insert(my_addr);
  }

  // Запускаем серверный поток
  std::thread server(server_thread, my_port);

  // Получаем список пиров от тракера
  response = send_to_tracker("get_peers " + my_addr);
  std::cout << "Peers from tracker: " << response << std::endl;

  // Подключаемся к полученным пирам
  std::stringstream ss(response);
  std::string peer;
  while (ss >> peer) {
    if (peer != my_addr) {
      size_t colon = peer.find(':');
      if (colon != std::string::npos) {
        std::string addr = peer.substr(0, colon);
        int port = std::stoi(peer.substr(colon + 1));
        connect_to_peer(addr, port, my_addr);
      }
    }
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(15));

    response = send_to_tracker("get_peers " + my_addr);
    std::cout << "Updated peers from tracker: " << response << std::endl;

    std::stringstream ss2(response);
    std::string new_peer;
    while (ss2 >> new_peer) {
      // Проверяем, знаем ли уже этого пира
      bool already_know;
      {
        std::lock_guard<std::mutex> lock(peers_mutex);
        already_know = (know_peers.find(new_peer) != know_peers.end());
      }

      // Если новый и не я - подключаемся
      if (!already_know && new_peer != my_addr) {
        size_t colon = new_peer.find(':');
        if (colon != std::string::npos) {
          std::string addr = new_peer.substr(0, colon);
          int port = std::stoi(new_peer.substr(colon + 1));
          connect_to_peer(addr, port, my_addr);
        }

        // Добавляем в свой список
        std::lock_guard<std::mutex> lock(peers_mutex);
        know_peers.insert(new_peer);
      }
    }
  }

  server.join();
  return 0;
}
