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
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/bind.hpp>
#include "Protocol.hpp"
#include "Message.hpp"

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

    static std::vector<unsigned char> EncryptMessage(const std::string& message, EVP_PKEY* public_key);
    static std::string DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key);

    // Методы для вычисления и проверки контрольной суммы
    static std::string CalculateChecksum(const std::string& message);
    static bool VerifyChecksum(const std::string& message, const std::string& checksum);

private:
    void OnConnect(const boost::system::error_code& error);
    void ReadHandler(const boost::system::error_code& error);
    void WriteImpl(std::array<char, MAX_IP_PACK_SIZE> msg);
    void WriteHandler(const boost::system::error_code& error);
    void CloseImpl();
    static EVP_PKEY* LoadKeyFromFile(const std::string& key_file, bool is_private, const std::string& password = "");
    std::string GetPublicKeyFingerprint(EVP_PKEY* public_key);
    std::vector<unsigned char> SignMessage(const std::string& message, EVP_PKEY* private_key);
    bool VerifySignature(const std::string& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key);

    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    std::array<char, MAX_IP_PACK_SIZE> read_msg_;
    std::deque<std::array<char, MAX_IP_PACK_SIZE>> write_msgs_;
    std::array<char, MAX_NICKNAME> nickname_;
    EVP_PKEY* client_private_key_;
};
#endif // CLIENT_HPP
