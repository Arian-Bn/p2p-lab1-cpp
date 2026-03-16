#include <array>
#include <boost/asio.hpp>
#include <iostream>

int main() {
  try {
    std::cout << "Server starting..." << std::endl;

    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9000));
    std::cout << "Server listening on port 9000..." << std::endl;

    boost::asio::ip::tcp::socket socket(io_context);
    acceptor.accept(socket);
    std::cout << "Client connected!" << std::endl;

    std::array<char, 128> buf;
    boost::system::error_code error;

    std::size_t len = socket.read_some(boost::asio::buffer(buf), error);

    if (error == boost::asio::error::eof) {
      std::cout << "Connection closed by client" << std::endl;
    } else if (error) {
      throw boost::system::system_error(error);
    }

    std::cout << "Received: " << std::string(buf.data(), len) << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
