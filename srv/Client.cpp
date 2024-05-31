// Client.cpp
#include "Client.hpp"


Client::Client(const std::array<char, MAX_NICKNAME>& nickname,
               boost::asio::io_service& io_service,
               tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service), socket_(io_service) {
    strcpy(nickname_.data(), nickname.data());
    memset(read_msg_.data(), '\0', MAX_IP_PACK_SIZE);
    boost::asio::async_connect(socket_, endpoint_iterator, boost::bind(&Client::OnConnect, this, _1));
}

void Client::Write(const std::array<char, MAX_IP_PACK_SIZE>& msg) {
    io_service_.post(boost::bind(&Client::WriteImpl, this, msg));
}

void Client::Close() {
    io_service_.post(boost::bind(&Client::CloseImpl, this));
}

void Client::OnConnect(const boost::system::error_code& error) {
    if (!error) {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(nickname_, nickname_.size()),
                                 boost::bind(&Client::ReadHandler, this, _1));
    }
}

void Client::ReadHandler(const boost::system::error_code& error) {
    std::cout << read_msg_.data() << std::endl;
    if (!error) {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_, read_msg_.size()),
                                boost::bind(&Client::ReadHandler, this, _1));
    } else {
        CloseImpl();
    }
}

void Client::WriteImpl(std::array<char, MAX_IP_PACK_SIZE> msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                                 boost::bind(&Client::WriteHandler, this, _1));
    }
}

void Client::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                                     boost::bind(&Client::WriteHandler, this, _1));
        }
    } else {
        CloseImpl();
    }
}

void Client::CloseImpl() {
    socket_.close();
}
