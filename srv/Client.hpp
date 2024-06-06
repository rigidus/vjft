// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <deque>
#include <array>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <boost/bind.hpp>
#include "Protocol.hpp"

using namespace boost::placeholders;
using boost::asio::ip::tcp;

extern std::string client_private_key_file;
extern std::vector<std::string> recipient_public_key_files;


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

    EVP_PKEY* LoadPrivateKey(const std::string& key_file, const std::string& password);


    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    std::array<char, MAX_IP_PACK_SIZE> read_msg_;
    std::deque<std::array<char, MAX_IP_PACK_SIZE>> write_msgs_;
    std::array<char, MAX_NICKNAME> nickname_;

    EVP_PKEY* client_private_key_;
};

#endif // CLIENT_HPP
