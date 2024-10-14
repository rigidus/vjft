//ende.c

#include "defs.h"

#include "all_libs.h"

#include "crypt.h"

int main(int argc, char* argv[]) {

    // Инициализация OpenSSL
    OpenSSL_add_all_algorithms();

    // Загрузка ключей
    EVP_PKEY* private_key =
        load_key_from_file("alice_private_key.pem", 1, NULL);
    if (!private_key) {
        fprintf(stderr, "Failed to load private key\n");
        return 1;
    }
    EVP_PKEY* public_key =
        load_key_from_file("alice_public_key.pem", 0, NULL);
    if (!public_key) {
        fprintf(stderr, "Failed to load public key\n");
        EVP_PKEY_free(private_key);
        return 1;
    }

    // Исходное сообщение
    char* original_msg = "Привет, Мир";
    size_t original_len = strlen(original_msg);

    // Шифрование сообщения
    size_t encrypted_len = 0;
    unsigned char* encrypted_msg =
        encipher(private_key, public_key, original_msg, original_len,
                 &encrypted_len);
    if (!encrypted_msg) {
        fprintf(stderr, "Encryption failed\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_free(public_key);
        return 1;
    }

    // Вывод зашифрованного сообщения в шестнадцатеричном формате
    /* LOG_HEX("Encrypted message", encrypted_msg, encrypted_len); */

    // Дешифрование сообщения
    unsigned char* decrypted_msg =
        decipher(private_key, public_key, encrypted_msg, encrypted_len);
    if (!decrypted_msg) {
        fprintf(stderr, "Decryption failed\n");
        free(encrypted_msg);
        EVP_PKEY_free(private_key);
        EVP_PKEY_free(public_key);
        return 1;
    }

    // Вывод расшифрованного сообщения
    printf("Decrypted message: %s\n", decrypted_msg);

    // Освобождение ресурсов
    /* free(encrypted_msg); */
    /* free(decrypted_msg); */
    /* EVP_PKEY_free(private_key); */
    /* EVP_PKEY_free(public_key); */

    return 0;
}
