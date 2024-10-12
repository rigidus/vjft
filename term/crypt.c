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


int calc_crc(const char* msg, size_t msg_len,
            unsigned char* hash) {
    // Инициализация хэша нулями
    memset(hash, 0, HASH_SIZE);

    // Создание нового контекста для вычисления хэша
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        fprintf(stderr,
                ":> calcCRC(): Error: Failed to create EVP_MD_CTX\n");
        return -1;
    }

    // Инициализация контекста для SHA-256
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr,
                ":> calcCRC(): Error: Failed to initialize digest\n");
        return -1;
    }

    // Обновление хэша данными сообщения
    if (EVP_DigestUpdate(mdctx, msg, msg_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr,
                ":> calcCRC(): Error: Failed to update digest\n");
        return -1;
    }

    // Завершение вычисления хэша
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr,
                ":> calcCRC(): Error: Failed to finalize digest\n");
        return -1;
    }

    // Освобождение контекста
    EVP_MD_CTX_free(mdctx);

    // Проверка, что длина хэша соответствует ожидаемой
    if (hash_len != HASH_SIZE) {
        fprintf(stderr,
                ":> calcCRC(): Warning: Unexpected hash length\n");
        return -1;
    }

    return 0;
}


int calc_sign(const char* msg, EVP_PKEY* key,
              size_t* sign_len, unsigned char** sign_ret)
{
    // Create the Message Digest Context
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    if (mdctx == NULL) {
        fprintf(stderr, "Err: Sign: Failed to create EVP_MD_CTX\n");
        return -1;
    }

    // Initialise the DigestSign operation - SHA-256
    // has been selected as the message digest function
    if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(),
                               NULL, key)) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr, "Err: Sign: initializing signing: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    // Call update with the message
    if (1 != EVP_DigestSignUpdate(mdctx, msg,
                                 strlen(msg))) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr, "Err: Sign: updating signing: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    // Finalise the DigestSign operation
    // First call EVP_DigestSignFinal with a NULL
    // sig parameter to obtain the length of the
    // signature. Length is returned in slen
    if (1 != EVP_DigestSignFinal(mdctx, NULL, sign_len)) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr, "Err: Sign: finalizing signing: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    // Allocate memory for the signature
    // based on size in slen
    unsigned char* sign = OPENSSL_malloc(*sign_len);
    if (!sign) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr, "Err: Sign: allocate signature buffer\n");
        return -1;
    }

    // Obtain the signature
    if (1 != EVP_DigestSignFinal(mdctx, sign, sign_len)) {
        EVP_MD_CTX_free(mdctx);
        OPENSSL_free(sign);
        sign = NULL;
        fprintf(stderr, "Err: Sign: getting signature: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    // Success
    EVP_MD_CTX_free(mdctx);
    *sign_ret = sign;
    /* after use we need make OPENSSL_free(sign); */
    return 1;
}


unsigned char* encipher(EVP_PKEY* private_key,
                        EVP_PKEY* public_key,
                        const char* msg,
                        int msg_len,
                        size_t* out_len) {
    // rnd_arr - array of unsigned char of random length
    // from 0 to 255 and random content, which is
    // necessary to make it impossible to guess the
    // meaning of the message by its length (for
    // example, for short messages like "yes" or "no").
    srand(time(NULL));
    unsigned int rnd_size = rand() % 256;
    if (rnd_size == 0) {
        rnd_size = 255;
    }
    unsigned char *rnd_arr =
        malloc(rnd_size * sizeof(unsigned char));
    if (rnd_arr == NULL) {
        perror("Err: enchipher memory allocation");
        exit(-1);
    }
    for (unsigned int i = 0; i < rnd_size; i++) {
        rnd_arr[i] = rand() % 256;
    }
    // now we have rnd_size and rnd_arr

    // Calculate msg_size in bytes
    uint16_t msg_size = msg_len;
    if (msg_size != msg_len) {
        perror("Err: msg too long");
        exit(-1);
    }

    // Calculate msg_crc
    unsigned char *msg_crc =
        malloc(HASH_SIZE * sizeof(unsigned char));
    if (-1 == calc_crc(msg, msg_len, msg_crc)) {
        perror("Err: bad crc");
        exit(-1);
    }

    // Calculate msg_sign
    size_t sign_len = 0;
    unsigned char *msg_sign = NULL;
    if (-1 == calc_sign(msg, private_key, &sign_len, &msg_sign)) {
        perror("Err: bad sign");
        exit(-1);
    }

    // Calculate envelope size
    /** Envelope:
        +---------------+
        | rnd_size      | 1 byte
        +---------------+
        | rnd_arr...    | 1..255 bytes
        +---------------+
        | msg_size      | 2 bytes
        +---------------+
        | msg_crc       | 32 bytes (HASH_SIZE)
        +---------------+
        | msg_sign      | 512 bytes (SIG_SIZE)
        +---------------+
        | msg...        | msg_size bytes
        +---------------+
    */
    int env_size =
        1 + rnd_size + 2 + HASH_SIZE + SIG_SIZE + msg_size;

    // Allocate envelope buffer
    unsigned char *env =
        malloc(env_size * sizeof(unsigned char));

    // Write to envelope
    uint8_t rnd_size_byte = rnd_size;
    memcpy(env, &(rnd_size_byte), 1);
    memcpy(env+1, rnd_arr, rnd_size);
    memcpy(env+1+rnd_size, &msg_size, 2);
    memcpy(env+1+rnd_size+2, msg_crc, HASH_SIZE);
    memcpy(env+1+rnd_size+HASH_SIZE, msg_sign, SIG_SIZE);
    memcpy(env+1+rnd_size+HASH_SIZE+SIG_SIZE, msg, msg_size);

    // [TODO:gmm] Continue here...
    // split_chunks
    // encrypt_chunks
    // make_pack
    // free memory for signature
    // and return

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
