// Crypt.cpp
#include "Crypt.hpp"
#include <openssl/err.h>
#include <openssl/pem.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>

std::array<unsigned char, HASH_SIZE> Crypt::calcCRC(
    const std::string& message)
{
    std::array<unsigned char, HASH_SIZE> hash = {};
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "Error Calc CRC: Failed to create EVP_MD_CTX" << std::endl;
        return hash;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Error Calc CRC: Failed to initialize digest" << std::endl;
        return hash;
    }

    if (EVP_DigestUpdate(mdctx, message.c_str(), message.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Error Calc CRC: Failed to update digest" << std::endl;
        return hash;
    }

    unsigned int lengthOfHash = 0;
    if (EVP_DigestFinal_ex(mdctx, hash.data(), &lengthOfHash) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Error Calc CRC: Failed to finalize digest" << std::endl;
        return hash;
    }

    EVP_MD_CTX_free(mdctx);
    return hash;
}


bool Crypt::verifyChecksum(const std::string& message,
                           const std::array<unsigned char, HASH_SIZE>& checksum)
{
    std::array<unsigned char, HASH_SIZE> calculatedChecksum = calcCRC(message);
    return calculatedChecksum == checksum;
}


std::optional<std::array<unsigned char, SIG_SIZE>> Crypt::sign(
    const std::string& message, EVP_PKEY* private_key)
{
    std::array<unsigned char, 512> signature = {};
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "Signing error: Failed to create EVP_MD_CTX" << std::endl;
        return std::nullopt;
    }

    if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error initializing signing: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return std::nullopt;
    }

    if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error updating signing: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    size_t siglen;
    if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error finalizing signing: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    if (siglen != signature.size()) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Unexpected signature size" << std::endl;
        return std::nullopt;
    }

    if (EVP_DigestSignFinal(mdctx, signature.data(), &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error getting signature: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    EVP_MD_CTX_free(mdctx);

    return signature;
}


bool Crypt::verify(
    const std::string& message, const std::array<unsigned char, 512>& signature,
    EVP_PKEY* public_key)
{
    // Создаем контекст для проверки подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "VerifySignature error: Failed to create EVP_MD_CTX" << std::endl;
        return false;
    }

    // Инициализируем контекст для проверки подписи с использованием алгоритма SHA-256
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error initializing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error updating verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Проверяем подпись
    int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    EVP_MD_CTX_free(mdctx);

    if (ret < 0) {
        std::cerr << "VerifySignature error: Error finalizing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Возвращаем true, если подпись верна, иначе false
    return ret == 1;
}

std::optional<std::vector<unsigned char>> Crypt::Encrypt(
    const std::vector<unsigned char>& packed, EVP_PKEY* public_key)
{
    size_t encrypted_length = 0;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, nullptr);
    if (!ctx) {
        std::cerr << "Encrypt error: Error creating context for encryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error initializing encryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_encrypt(ctx, nullptr, &encrypted_length, packed.data(), packed.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error determining buffer length for encryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> encrypted(encrypted_length);
    if (EVP_PKEY_encrypt(ctx, encrypted.data(), &encrypted_length, packed.data(), packed.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error encrypting message" << std::endl;
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return encrypted;
}


std::optional<std::vector<unsigned char>> Crypt::Decrypt(
    const std::vector<unsigned char>& encrypted_chunk, EVP_PKEY* private_key)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
        std::cerr << "Decrypt error: Error creating context for decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error initializing decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_chunk.data(), encrypted_chunk.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error determining buffer length for decryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_chunk.data(), encrypted_chunk.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error decrypting chunk" << std::endl;
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return out;
}


std::string Crypt::Base64Encode(const std::vector<unsigned char>& buffer) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    bio = BIO_push(bio, bmem);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer.data(), buffer.size());
    BIO_flush(bio);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);

    std::string encoded(bptr->data, bptr->length);
    BIO_free_all(bio);

    return encoded;
}

std::vector<unsigned char> Crypt::Base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), encoded.size());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    std::vector<unsigned char> buffer(encoded.size());
    int decoded_length = BIO_read(bio, buffer.data(), buffer.size());
    buffer.resize(decoded_length);
    BIO_free_all(bio);

    return buffer;
}

std::optional<std::string> Crypt::DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
        std::cerr << "DecryptMessage error: Error creating context for decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error initializing decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error determining buffer length for decryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error decrypting message" << std::endl;
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return std::string(out.begin(), out.end());
}

bool Crypt::VerifySignature(const std::string& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "VerifySignature error: Failed to create EVP_MD_CTX" << std::endl;
        return false;
    }

    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error initializing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error updating verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    EVP_MD_CTX_free(mdctx);

    if (ret < 0) {
        std::cerr << "VerifySignature error: Error finalizing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    return ret == 1;
}
