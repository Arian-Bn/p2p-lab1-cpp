#include <array>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/registered_buffer.hpp>
#include <iostream>

int main() {
  try {
    std::cout << "Third service (relay) starting..." << std::endl;

    boost::asio::io_context io_context;

    // Слушает порт 9002
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9002));
    std::cout << "Relay listening on port 9002..." << std::endl;

    // Принимаем клиента (первый микросервис)
    boost::asio::ip::tcp::socket client_socket(io_context);
    acceptor.accept(client_socket);
    std::cout << "Client connected to relay!" << std::endl;

    // Подключаемся к серверу (второй микросервис)
    boost::asio::ip::tcp::socket server_socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoint = resolver.resolve("127.0.0.1", "9000");
    boost::asio::connect(server_socket, endpoint);
    std::cout << "Relay connected to server!" << std::endl;

    // Получаем сообщение от клиента
    std::array<char, 128> buf;
    boost::system::error_code error;

    size_t len = client_socket.read_some(boost::asio::buffer(buf), error);
    std::cout << "Relay received: " << std::string(buf.data(), len)
              << std::endl;

    // Пересылаем серверу
    boost::asio::write(server_socket, boost::asio::buffer(buf, len));

    // Получаем ответ от сервера
    len = server_socket.read_some(boost::asio::buffer(buf), error);
    std::cout << "Relay got response: " << std::string(buf.data(), len)
              << std::endl;

    // Пересылаем клиенту
    boost::asio::write(client_socket, boost::asio::buffer(buf, len));
    std::cout << "Relay forwarded response to client" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
