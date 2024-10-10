#include "crypt.h"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

EVP_PKEY* load_key_from_file(const char* key_file, int is_private, const char* password) {
    FILE* fp = fopen(key_file, "r");
    if (!fp) {
        fprintf(stderr, "Error opening key file: %s\n", key_file);
        return NULL;
    }

    EVP_PKEY* key = NULL;
    if (is_private) {
        key = PEM_read_PrivateKey(fp, NULL, NULL, (void*)password);
    } else {
        key = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    }
    fclose(fp);

    if (!key) {
        fprintf(stderr, "Error loading key: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return NULL;
    }

    return key;
}

// Реализация функций encipher и decipher, адаптированная на C

unsigned char* encipher(EVP_PKEY* private_key, EVP_PKEY* public_key, const unsigned char* msg, size_t msg_len, size_t* out_len) {
    // Реализация функции шифрования на C
    // Используйте EVP_PKEY_encrypt и связанные функции из OpenSSL
    // Верните зашифрованные данные и их длину через out_len
    // Не забудьте освобождать выделенную память после использования
    // Здесь предоставлен упрощенный пример, подробная реализация зависит от ваших требований

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, NULL);
    if (!ctx) {
        fprintf(stderr, "Error creating context for encryption\n");
        return NULL;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        fprintf(stderr, "Error initializing encryption\n");
        return NULL;
    }

    // Настройте параметры шифрования по необходимости, например, RSA padding

    size_t encrypted_len = 0;
    if (EVP_PKEY_encrypt(ctx, NULL, &encrypted_len, msg, msg_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        fprintf(stderr, "Error determining buffer length for encryption\n");
        return NULL;
    }

    unsigned char* encrypted_data = (unsigned char*)malloc(encrypted_len);
    if (!encrypted_data) {
        EVP_PKEY_CTX_free(ctx);
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    if (EVP_PKEY_encrypt(ctx, encrypted_data, &encrypted_len, msg, msg_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        free(encrypted_data);
        fprintf(stderr, "Error encrypting message\n");
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);
    *out_len = encrypted_len;
    return encrypted_data;
}

unsigned char* decipher(EVP_PKEY* private_key, EVP_PKEY* public_key, const unsigned char* encrypted_data, size_t encrypted_len, size_t* out_len) {
    // Реализация функции дешифрования на C
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, NULL);
    if (!ctx) {
        fprintf(stderr, "Error creating context for decryption\n");
        return NULL;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        fprintf(stderr, "Error initializing decryption\n");
        return NULL;
    }

    // Настройте параметры дешифрования по необходимости

    size_t decrypted_len = 0;
    if (EVP_PKEY_decrypt(ctx, NULL, &decrypted_len, encrypted_data, encrypted_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        fprintf(stderr, "Error determining buffer length for decryption\n");
        return NULL;
    }

    unsigned char* decrypted_data = (unsigned char*)malloc(decrypted_len + 1); // +1 для нулевого терминатора
    if (!decrypted_data) {
        EVP_PKEY_CTX_free(ctx);
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    if (EVP_PKEY_decrypt(ctx, decrypted_data, &decrypted_len, encrypted_data, encrypted_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        free(decrypted_data);
        fprintf(stderr, "Error decrypting message\n");
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);
    decrypted_data[decrypted_len] = '\0'; // Нулевой терминатор для удобства
    *out_len = decrypted_len;
    return decrypted_data;
}
