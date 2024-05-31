// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <deque>
#include <array>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "Protocol.hpp"

#include <boost/bind.hpp>
using namespace boost::placeholders;


using boost::asio::ip::tcp;

class Client {
public:
    Client(const std::array<char, MAX_NICKNAME>& nickname,
           boost::asio::io_service& io_service,
           tcp::resolver::iterator endpoint_iterator);
    void Write(const std::array<char, MAX_IP_PACK_SIZE>& msg);
    void Close();

private:
    void OnConnect(const boost::system::error_code& error);
    void ReadHandler(const boost::system::error_code& error);
    void WriteImpl(std::array<char, MAX_IP_PACK_SIZE> msg);
    void WriteHandler(const boost::system::error_code& error);
    void CloseImpl();

    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    std::array<char, MAX_IP_PACK_SIZE> read_msg_;
    std::deque<std::array<char, MAX_IP_PACK_SIZE>> write_msgs_;
    std::array<char, MAX_NICKNAME> nickname_;
};

#endif // CLIENT_HPP
