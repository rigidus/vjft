// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <deque>
#include <array>
#include <iostream>
#include <optional>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/bind.hpp>
#include "Protocol.hpp"
#include "Message.hpp"
#include "Crypt.hpp"
#include "Utils.hpp"
#include "Log.hpp"

using namespace boost::placeholders;
using boost::asio::ip::tcp;

extern std::string client_private_key_file;
extern std::vector<std::string> recipient_public_key_files;

class Client {
public:
    Client(const std::array<char, MAX_NICKNAME>& nickname,
           boost::asio::io_service& io_service,
           tcp::resolver::iterator endpoint_iterator);
    void Write(const std::vector<unsigned char>& msg);
    void Close();

private:
    void OnConnect(const boost::system::error_code& error);
    void FindSyncMarker();
    bool checkSyncMarkerInQueue();
    void HandleDataTimeout(const boost::system::error_code& error);
    void Recover();
    void HeaderHandler(const boost::system::error_code& error);
    void ReadHandler(const boost::system::error_code& error, size_t bytes_readed);
    void WriteImpl(std::vector<unsigned char> msg);
    void WriteHandler(const boost::system::error_code& error);
    void CloseImpl();

    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    std::vector<unsigned char> read_msg_;
    std::deque<std::vector<unsigned char>> write_msgs_;
    std::array<char, MAX_NICKNAME> nickname_;
    EVP_PKEY* client_private_key_;
    std::vector<EVP_PKEY*> recipient_public_keys;
    std::vector<std::string> recipient_public_keys_fingerprints;
    size_t zero_byte_count_;
    boost::asio::deadline_timer read_timeout_timer_;
    std::deque<unsigned char> read_queue_;
};
#endif // CLIENT_HPP
