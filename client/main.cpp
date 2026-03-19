#include <array>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/registered_buffer.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  try {
    std::cout << "Client starting..." << std::endl;

    std::string request;
    if (argc == 1) {
      request = "get_all";
      std::cout << "No arguments, using default" << std::endl;
    } else if (argc == 2) {
      request = argv[1];
      std::cout << "Single argument: " << request << std::endl;
    } else if (argc == 3) {
      request = std::string(argv[1]) + " " + std::string(argv[2]);
    } else {
      std::cout << "too many arguments. Usage: ./client [command] [parameter]"
                << std::endl;
      return 1;
    }

    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoint = resolver.resolve("127.0.0.1", "9002");

    boost::asio::connect(socket, endpoint);
    std::cout << "Connected to server!" << std::endl;

    boost::asio::write(socket, boost::asio::buffer(request));
    std::cout << "Request sent: " << request << std::endl;

    std::array<char, 512> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);

    if (!error) {
      std::cout << "Received: " << std::string(buf.data(), len) << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
