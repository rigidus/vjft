#include "CryptoHelper.hpp"
#include <stdexcept>

std::vector<unsigned char> CryptoHelper::EncryptMessage(const std::string& message, EVP_PKEY* public_key) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, nullptr);
    if (!ctx) {
        throw std::runtime_error("Error creating context for encryption");
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error initializing encryption");
    }

    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, reinterpret_cast<const unsigned char*>(message.c_str()), message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error determining buffer length for encryption");
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_encrypt(ctx, out.data(), &outlen, reinterpret_cast<const unsigned char*>(message.c_str()), message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error encrypting message");
    }

    EVP_PKEY_CTX_free(ctx);
    return out;
}

    std::string CryptoHelper::DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key) {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
        if (!ctx) {
            throw std::runtime_error("Error creating context for decryption");
        }

        if (EVP_PKEY_decrypt_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Error initializing decryption");
        }

        size_t outlen;
        if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Error determining buffer length for decryption");
        }

        std::vector<unsigned char> out(outlen);
        if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            throw std::runtime_error("Error decrypting message");
        }

        EVP_PKEY_CTX_free(ctx);
        return std::string(out.begin(), out.end());
    }
