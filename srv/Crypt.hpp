// Crypt.hpp
#ifndef CRYPT_HPP
#define CRYPT_HPP

#include <openssl/evp.h>
#include <string>
#include <vector>
#include <optional>
#include <array>

#include "defs.hpp"
#include <openssl/err.h>
#include <openssl/pem.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include "Utils.hpp"

class Crypt {
public:
    static std::array<unsigned char, HASH_SIZE> calcCRC(
        const std::string& message);

    static bool verifyChecksum(const std::string& message,
                               const std::array<unsigned char, HASH_SIZE>& checksum);

    static std::optional<std::array<unsigned char, SIG_SIZE>> sign(
        const std::string& message, EVP_PKEY* private_key);

    static std::optional<std::vector<unsigned char>> Encrypt(
        const std::vector<unsigned char>& packed, EVP_PKEY* public_key);

    static std::optional<std::vector<unsigned char>> Decrypt(
        const std::vector<unsigned char>& encrypted_chunk, EVP_PKEY* private_key);

    static std::string Base64Encode(
        const std::vector<unsigned char>& buffer);

    static std::vector<unsigned char> Base64Decode(const std::string& encoded);

    static std::optional<std::string> DecryptMessage(
        const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key);

    static std::optional<std::vector<unsigned char>> SignMsg(
        const std::string& message, EVP_PKEY* private_key);

    static bool VerifySignature(
        const std::string& message,
        const std::vector<unsigned char>& signature, EVP_PKEY* public_key);
    // static bool VerifyChecksum(
    //     const std::string& message,
    //     const std::array<unsigned char, EVP_MAX_MD_SIZE> crc);

    static std::vector<unsigned char> encipher(
        EVP_PKEY* private_key, EVP_PKEY* public_key, std::string msg);
    static std::string decipher(
        EVP_PKEY* private_key, EVP_PKEY* public_key, std::vector<unsigned char> pack);
};

#endif // CRYPT_HPP
