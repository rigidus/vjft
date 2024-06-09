// test_crypto.cpp
#include "test_crypto.hpp"
#include <openssl/err.h>

bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key) {
    std::string original_message = "This is a test message.";
    std::string checksum;
    std::map<std::string, std::string> data;
    std::string packed_message;
    std::optional<std::vector<unsigned char>> encrypted_optional;
    std::vector<unsigned char> encrypted_message;
    std::string base64_encrypted_message;
    std::map<std::string, std::string> envelope;
    bool result = false;

    // Контрольная сумма сообщения
    checksum = Client::CalculateChecksum(original_message);

    // Подписание сообщения
    std::string signature = "";
    auto optSign = Client::SignMsg(original_message, private_key);
    if (!optSign) {
        return result;
    }
    signature = Client::Base64Encode(*optSign);

    // Упаковка сообщения
    data = {
        {"crc", checksum},
        {"msg", original_message},
        {"sign", signature}
    };
    for (const auto& pair : data) { // dbgout
        std::cout << "\n" << pair.first << ": " << pair.second << std::endl;
    }
    packed_message = Message::pack(data);
    std::cout << "\n" << "packed_message: " << packed_message << std::endl; // dbgout

    // Шифрование упакованного сообщения
    encrypted_optional = Client::EncryptMessage(packed_message, public_key);
    if (!(encrypted_optional)) {
        std::cerr << "Error: Encryption Failed" << std::endl;
        return result;
    }
    encrypted_message = *encrypted_optional;

    base64_encrypted_message = Client::Base64Encode(encrypted_message);

    envelope = {
        {"beem", base64_encrypted_message}
    };

    for (const auto& pair : envelope) { // dbgout
        std::cout << "\n" << pair.first << ": " << pair.second << std::endl;
    }

    // ------------------------


    // Декодирование из Base64
    std::vector<unsigned char> decoded_message =
        Client::Base64Decode(base64_encrypted_message);

    // Расшифровывание сообщения
    std::optional<std::string> decrypted_optional = Client::DecryptMessage(decoded_message, private_key);
    if (!decrypted_optional) {
        std::cerr << "Error: Decryption Failed" << std::endl;
        return result;
    }
    std::string decrypted_message = *decrypted_optional;

    std::cout << "\ndecrypted_message: " << decrypted_message << std::endl;

    // Распаковка сообщения
    std::map<std::string, std::string> unpacked_data = Message::unpack(decrypted_message);
    for (const auto& pair : data) { // dbgout
        std::cout << "\n" << pair.first << ": " << pair.second << std::endl;
    }

    // Проверка контрольной суммы
    std::string unpacked_message = unpacked_data["msg"];
    std::string unpacked_checksum = unpacked_data["crc"];
    if (!Client::VerifyChecksum(unpacked_message, unpacked_checksum)) {
        std::cerr << "Error: Checksum verification failed" << std::endl;
        return result;
    }

    // Проверка подписи
    std::vector<unsigned char> unpacked_signature =
        Client::Base64Decode(unpacked_data["sign"]);
    std::cout << "Original Signature: "
              << Client::Base64Encode(*optSign) << std::endl;
    std::cout << "Unpacked Signature: "
              << Client::Base64Encode(unpacked_signature) << std::endl;
    if (*optSign == unpacked_signature) {
        std::cerr << "Signature equ!!!" << std::endl;
    } else {
        std::cerr << "sign1: " << unpacked_data["sign"] << std::endl;
        std::cerr << "sign2: " << signature << std::endl;

    }
    if (!Client::VerifySignature(unpacked_message, unpacked_signature, public_key)) {
        std::cerr << "Error: Signature verification failed" << std::endl;
        return result;
    }

    // Если все проверки прошли успешно
    result = (original_message == unpacked_message);

    std::cout << "\nTest Full Sequence: " << std::endl;

    return result;
}

int main() {
    std::string private_key_file = "client_private_key.pem";
    std::string public_key_file = "client_public_key.pem";
    std::string password;

    std::cout << "Enter password for private key: ";
    std::cin >> password;

    EVP_PKEY* private_key = Client::LoadKeyFromFile(private_key_file, true, password);
    EVP_PKEY* public_key = Client::LoadKeyFromFile(public_key_file, false);

    if (!private_key || !public_key) {
        std::cerr << "\nTest Full Sequence: Error |: No key files" << std::endl;
        return 1;
    }

    bool full_sequence_result = TestFullSequence(private_key, public_key);

    std::cout << "\nTest Full Sequence: "
    << (full_sequence_result ? "PASSED" : "FAILED") << std::endl;

    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);

    return 0;
}



// // test_crypto.cpp
// #include "Client.hpp"
// #include <iostream>
// #include <vector>
// #include <openssl/evp.h>
// #include <openssl/pem.h>
// #include <openssl/err.h>

// // EVP_PKEY* LoadKeyFromFile(const std::string& key_file, bool is_private, const std::string& password = "") {
// //     FILE* fp = fopen(key_file.c_str(), "r");
// //     if (!fp) {
// //         std::cerr << "Error opening key file: " << key_file << std::endl;
// //         return nullptr;
// //     }

// //     EVP_PKEY* key = nullptr;
// //     if (is_private) {
// //         key = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char*>(password.c_str()));
// //     } else {
// //         key = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
// //     }
// //     fclose(fp);

// //     if (!key) {
// //         std::cerr << "Error loading key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
// //     }

// //     return key;
// // }

// // // Функция для подписывания сообщения
// // std::vector<unsigned char> SignMessage(const std::string& message, EVP_PKEY* private_key) {
// //     EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
// //     if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
// //         EVP_MD_CTX_free(mdctx);
// //         throw std::runtime_error("Error initializing signing");
// //     }

// //     if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
// //         EVP_MD_CTX_free(mdctx);
// //         throw std::runtime_error("Error updating signing");
// //     }

// //     size_t siglen;
// //     if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
// //         EVP_MD_CTX_free(mdctx);
// //         throw std::runtime_error("Error finalizing signing");
// //     }

// //     std::vector<unsigned char> sig(siglen);
// //     if (EVP_DigestSignFinal(mdctx, sig.data(), &siglen) <= 0) {
// //         EVP_MD_CTX_free(mdctx);
// //         throw std::runtime_error("Error getting signature");
// //     }

// //     EVP_MD_CTX_free(mdctx);
// //     return sig;
// // }

// // // Функция для проверки подписи
// //     bool VerifySignature(const std::string& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) {
// //         EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
// //         if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
// //             EVP_MD_CTX_free(mdctx);
// //             throw std::runtime_error("Error initializing verification");
// //         }

// //         if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
// //             EVP_MD_CTX_free(mdctx);
// //             throw std::runtime_error("Error updating verification");
// //         }

// //         int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
// //         EVP_MD_CTX_free(mdctx);
// //         return ret == 1;
// //     }

// int main() {
//     // // Загрузка ключей
//     // std::string private_key_file = "client_private_key.pem";
//     // std::string public_key_file = "client_public_key.pem";
//     // std::string password;

//     // std::cout << "Enter password for private key: ";
//     // std::cin >> password;

//     // EVP_PKEY* private_key = LoadKeyFromFile(private_key_file, true, password);
//     // EVP_PKEY* public_key = LoadKeyFromFile(public_key_file, false);

//     // if (!private_key || !public_key) {
//     //     return 1;
//     // }

//     // // Создание тестового сообщения
//     // std::string message = "This is a test message for encryption and decryption.";

//     // // Шифрование сообщения
//     // std::vector<unsigned char> encrypted_message;
//     // try {
//     //     encrypted_message = Client::EncryptMessage(message, public_key);
//     //     std::cout << "Message encrypted successfully." << std::endl;
//     // } catch (const std::exception& e) {
//     //     std::cerr << "Encryption error: " << e.what() << std::endl;
//     //     EVP_PKEY_free(private_key);
//     //     EVP_PKEY_free(public_key);
//     //     return 1;
//     // }

//     // // Подписывание сообщения
//     // std::vector<unsigned char> signature;
//     // try {
//     //     signature = SignMessage(message, private_key);
//     //     std::cout << "Message signed successfully." << std::endl;
//     // } catch (const std::exception& e) {
//     //     std::cerr << "Signing error: " << e.what() << std::endl;
//     //     EVP_PKEY_free(private_key);
//     //     EVP_PKEY_free(public_key);
//     //     return 1;
//     // }

//     // // Расшифровывание сообщения
//     // std::string decrypted_message;
//     // try {
//     //     decrypted_message = Client::DecryptMessage(encrypted_message, private_key);
//     //     std::cout << "Message decrypted successfully: " << decrypted_message << std::endl;
//     // } catch (const std::exception& e) {
//     //     std::cerr << "Decryption error: " << e.what() << std::endl;
//     //     EVP_PKEY_free(private_key);
//     //     EVP_PKEY_free(public_key);
//     //     return 1;
//     // }

//     // // Проверка подписи
//     // try {
//     //     if (VerifySignature(decrypted_message, signature, public_key)) {
//     //         std::cout << "Signature verified successfully." << std::endl;
//     //     } else {
//     //         std::cerr << "Signature verification failed." << std::endl;
//     //     }
//     // } catch (const std::exception& e) {
//     //     std::cerr << "Verification error: " << e.what() << std::endl;
//     //     EVP_PKEY_free(private_key);
//     //     EVP_PKEY_free(public_key);
//     //     return 1;
//     // }

//     // // Очистка ресурсов
//     // EVP_PKEY_free(private_key);
//     // EVP_PKEY_free(public_key);

//     return 0;
// }
