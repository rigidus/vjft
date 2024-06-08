// test_crypto.cpp
#include "Client.hpp"
#include <iostream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// EVP_PKEY* LoadKeyFromFile(const std::string& key_file, bool is_private, const std::string& password = "") {
//     FILE* fp = fopen(key_file.c_str(), "r");
//     if (!fp) {
//         std::cerr << "Error opening key file: " << key_file << std::endl;
//         return nullptr;
//     }

//     EVP_PKEY* key = nullptr;
//     if (is_private) {
//         key = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char*>(password.c_str()));
//     } else {
//         key = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
//     }
//     fclose(fp);

//     if (!key) {
//         std::cerr << "Error loading key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
//     }

//     return key;
// }

// // Функция для подписывания сообщения
// std::vector<unsigned char> SignMessage(const std::string& message, EVP_PKEY* private_key) {
//     EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
//     if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
//         EVP_MD_CTX_free(mdctx);
//         throw std::runtime_error("Error initializing signing");
//     }

//     if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
//         EVP_MD_CTX_free(mdctx);
//         throw std::runtime_error("Error updating signing");
//     }

//     size_t siglen;
//     if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
//         EVP_MD_CTX_free(mdctx);
//         throw std::runtime_error("Error finalizing signing");
//     }

//     std::vector<unsigned char> sig(siglen);
//     if (EVP_DigestSignFinal(mdctx, sig.data(), &siglen) <= 0) {
//         EVP_MD_CTX_free(mdctx);
//         throw std::runtime_error("Error getting signature");
//     }

//     EVP_MD_CTX_free(mdctx);
//     return sig;
// }

// // Функция для проверки подписи
//     bool VerifySignature(const std::string& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) {
//         EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
//         if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
//             EVP_MD_CTX_free(mdctx);
//             throw std::runtime_error("Error initializing verification");
//         }

//         if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
//             EVP_MD_CTX_free(mdctx);
//             throw std::runtime_error("Error updating verification");
//         }

//         int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
//         EVP_MD_CTX_free(mdctx);
//         return ret == 1;
//     }

int main() {
    // // Загрузка ключей
    // std::string private_key_file = "client_private_key.pem";
    // std::string public_key_file = "client_public_key.pem";
    // std::string password;

    // std::cout << "Enter password for private key: ";
    // std::cin >> password;

    // EVP_PKEY* private_key = LoadKeyFromFile(private_key_file, true, password);
    // EVP_PKEY* public_key = LoadKeyFromFile(public_key_file, false);

    // if (!private_key || !public_key) {
    //     return 1;
    // }

    // // Создание тестового сообщения
    // std::string message = "This is a test message for encryption and decryption.";

    // // Шифрование сообщения
    // std::vector<unsigned char> encrypted_message;
    // try {
    //     encrypted_message = Client::EncryptMessage(message, public_key);
    //     std::cout << "Message encrypted successfully." << std::endl;
    // } catch (const std::exception& e) {
    //     std::cerr << "Encryption error: " << e.what() << std::endl;
    //     EVP_PKEY_free(private_key);
    //     EVP_PKEY_free(public_key);
    //     return 1;
    // }

    // // Подписывание сообщения
    // std::vector<unsigned char> signature;
    // try {
    //     signature = SignMessage(message, private_key);
    //     std::cout << "Message signed successfully." << std::endl;
    // } catch (const std::exception& e) {
    //     std::cerr << "Signing error: " << e.what() << std::endl;
    //     EVP_PKEY_free(private_key);
    //     EVP_PKEY_free(public_key);
    //     return 1;
    // }

    // // Расшифровывание сообщения
    // std::string decrypted_message;
    // try {
    //     decrypted_message = Client::DecryptMessage(encrypted_message, private_key);
    //     std::cout << "Message decrypted successfully: " << decrypted_message << std::endl;
    // } catch (const std::exception& e) {
    //     std::cerr << "Decryption error: " << e.what() << std::endl;
    //     EVP_PKEY_free(private_key);
    //     EVP_PKEY_free(public_key);
    //     return 1;
    // }

    // // Проверка подписи
    // try {
    //     if (VerifySignature(decrypted_message, signature, public_key)) {
    //         std::cout << "Signature verified successfully." << std::endl;
    //     } else {
    //         std::cerr << "Signature verification failed." << std::endl;
    //     }
    // } catch (const std::exception& e) {
    //     std::cerr << "Verification error: " << e.what() << std::endl;
    //     EVP_PKEY_free(private_key);
    //     EVP_PKEY_free(public_key);
    //     return 1;
    // }

    // // Очистка ресурсов
    // EVP_PKEY_free(private_key);
    // EVP_PKEY_free(public_key);

    return 0;
}
