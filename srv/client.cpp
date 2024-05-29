#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void send_message(tcp::socket& socket, const std::string& message) {
    boost::asio::write(socket, boost::asio::buffer(message + "\n"));
}

std::string receive_message(tcp::socket& socket) {
    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, '\n');
    std::istream is(&buf);
    std::string message;
    std::getline(is, message);
    return message;
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: client <host> <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[1], argv[2]);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        tcp::socket socket(io_service);
        boost::asio::connect(socket, endpoint_iterator);

        std::cout << "Connected to the server. You can start sending messages.\n";

        std::string message;
        while (true) {
            std::cout << "Enter message (or 'exit' to quit): ";
            std::getline(std::cin, message);

            if (message == "exit")
                break;

            send_message(socket, message);
            std::string response = receive_message(socket);
            std::cout << "Server response: " << response << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
