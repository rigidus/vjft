#ifndef CRYPTOHELPER_HPP
#define CRYPTOHELPER_HPP

#include <vector>
#include <string>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

class CryptoHelper {
public:
    static std::vector<unsigned char> EncryptMessage(const std::string& message, EVP_PKEY* public_key);
    static std::string DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key);
};

#endif // CRYPTOHELPER_HPP
