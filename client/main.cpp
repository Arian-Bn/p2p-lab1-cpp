#include <array>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/registered_buffer.hpp>
#include <iostream>

int main() {
  try {
    std::cout << "Client starting..." << std::endl;

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoint = resolver.resolve("127.0.0.1", "9002");

    boost::asio::connect(socket, endpoint);
    std::cout << "Connected to server!" << std::endl;

    std::string message = "Hello from client1";
    boost::asio::write(socket, boost::asio::buffer(message));
    std::cout << "Sent: " << message << std::endl;

    std::array<char, 128> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);

    if (!error) {
      std::cout << "Received: " << std::string(buf.data(), len) << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "Errro: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
