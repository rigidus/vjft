// test_crypto.cpp
#include "test_crypto.hpp"
#include <openssl/err.h>

bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key) {
    std::string original_message = "This is a test message.";
    bool result = false;

    // Контрольная сумма сообщения
    std::string checksum = Client::CalculateChecksum(original_message);

    // Подписание сообщения
    std::string signature = "";
    auto optSign = Client::SignMsg(original_message, private_key);
    if (!optSign) {
        return result;
    }
    signature = Client::Base64Encode(*optSign);

    // Упаковка сообщения
    std::map<std::string, std::string> data = {
        {"crc", checksum},
        {"msg", original_message},
        {"sign", signature}
    };
    for (const auto& pair : data) { // dbgout
        std::cout << "\n" << pair.first << ": " << pair.second << std::endl;
    }
    std::string packed_message = Message::pack(data);
    std::cout << "\n" << "packed_message: " << packed_message << std::endl; // dbgout

    // Шифрование упакованного сообщения
    std::optional<std::vector<unsigned char>> encrypted_optional =
        Client::EncryptMessage(packed_message, public_key);
    if (!(encrypted_optional)) {
        std::cerr << "Error: Encryption Failed" << std::endl;
        return result;
    }
    std::vector<unsigned char> encrypted_message = *encrypted_optional;

    // Base64-кодирование упакованного сообщения
    std::string base64_encrypted_message = Client::Base64Encode(encrypted_message);

    // Помещаем base64 в хэшмпап-конверт для удобства
    std::map<std::string, std::string> envelope = {
        {"beem", base64_encrypted_message}
    };

    // И выводим для отладки
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

    // Отладочный вывод
    std::cout << "\ndecrypted_message: " << decrypted_message << std::endl;

    // Распаковка сообщения
    std::map<std::string, std::string> unpacked_data = Message::unpack(decrypted_message);
    for (const auto& pair : unpacked_data) { // dbgout
        std::cout << "\n%" << pair.first << ": " << pair.second << std::endl;
    }

    // Проверка контрольной суммы
    std::string unpacked_message = unpacked_data["msg"];
    std::string unpacked_checksum = unpacked_data["crc"];
    if (!Client::VerifyChecksum(unpacked_message, unpacked_checksum)) {
        std::cerr << "Error: Checksum verification failed" << std::endl;
        return result;
    }

    // Проверка что подпись после расшифровки совпадает
    std::string unpacked_signature = unpacked_data["sign"];
    std::cout << "Original Signature: " << signature << std::endl;
    std::cout << "Unpacked Signature: " << unpacked_signature << std::endl;
    if (signature != unpacked_signature) {
        std::cerr << "Signature NOT EQU" << std::endl;
        return result;
    }
    std::cerr << "Signature EQU" << std::endl;

    // Криптографическая проверка подписи
    if (!Client::VerifySignature(
            unpacked_message, Client::Base64Decode(unpacked_signature), public_key)) {
        std::cerr << "Error: Signature verification failed" << std::endl;
        return result;
    }

    // Если все проверки прошли успешно и сообщения равны
    result = (original_message == unpacked_message);

    std::cout << "\nTestFullSequence End" << std::endl;

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
