#include <array>
#include <boost/asio.hpp>
#include <boost/asio/registered_buffer.hpp>
#include <iostream>
#include <map>
#include <string>

// Простая бд параметров
std::map<std::string, std::string> get_mparams() {
  return {{"remperature", "22.5"},
          {"humidity", "65%"},
          {"status", "online"},
          {"version", "1.0.3"}};
}

int main() {
  try {
    std::cout << "Server starting..." << std::endl;

    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9000));
    std::cout << "Server listening on port 9000..." << std::endl;

    while (true) {
      boost::asio::ip::tcp::socket socket(io_context);
      acceptor.accept(socket);
      std::cout << "Client connected!" << std::endl;

      // Читаем запрос от клиента
      std::array<char, 128> buf;
      boost::system::error_code error;
      std::size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (error == boost::asio::error::eof) {
        std::cout << "Connection closed by client" << std::endl;
        continue;
      } else if (error) {
        throw boost::system::system_error(error);
      }

      std::string request(buf.data(), len);
      std::cout << "Received: " << request << std::endl;

      // Обрабатываем запрос
      std::string response;
      auto params = get_mparams();

      if (request == "get_all") {
        // Вернуть все параметры
        for (const auto &[key, value] : params) {
          response += key + " = " + value + "\n";
        }
      } else if (request.find("get ") == 0) {
        // Запрос конкретного параметра
        std::string param_name = request.substr(4);
        auto it = params.find(param_name);
        if (it != params.end()) {
          response = it->second;
        } else {
          response = "Unknown command. Available: get_all, get <param>";
        }
      }

      // Отправляем ответ
      boost::asio::write(socket, boost::asio::buffer(response));
      std::cout << "Response sent: " << response << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
