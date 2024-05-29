#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include "database.hpp"

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Database& db) : socket_(std::move(socket)), db_(db) {}

    void start() {
        read();
    }

private:
    void read() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(data_), '\n',
                                      [this, self](boost::system::error_code ec, std::size_t length) {
                                          if (!ec) {
                                              std::string message(data_.substr(0, length));
                                              data_.erase(0, length);
                                              handle_message(message);
                                              read();
                                          }
                                      });
    }

    void handle_message(const std::string& message) {
        // Сохраняем сообщение в базе данных
        db_.storeMessage("anonymous", message);

        // Обработка сообщения (например, отправка другим пользователям)
        std::cout << "Received message: " << message << std::endl;
    }

    tcp::socket socket_;
    std::string data_;
    Database& db_;
};

class ChatServer {
public:
    ChatServer(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), socket_(io_service), db_("chat.db") {
        start_accept();
    }

private:
    void start_accept() {
        acceptor_.async_accept(socket_, boost::bind(&ChatServer::handle_accept, this, boost::asio::placeholders::error));
    }

    void handle_accept(const boost::system::error_code& error) {
        if (!error) {
            std::make_shared<Session>(std::move(socket_), db_)->start();
        }
        start_accept();
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    Database db_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: ChatServer <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        ChatServer server(io_service, std::atoi(argv[1]));
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
