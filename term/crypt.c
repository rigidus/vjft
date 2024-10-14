#include "crypt.h"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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


unsigned char** split_buffer(const unsigned char* buffer,
                             size_t buffer_length,
                             size_t chunk_size,
                             size_t* num_chunks) {
    if (buffer == NULL || chunk_size == 0 || num_chunks == NULL) {
        // Некорректные параметры
        return NULL;
    }

    // Вычисляем количество чанков
    *num_chunks = buffer_length / chunk_size;
    if (buffer_length % chunk_size != 0) {
        (*num_chunks)++;
    }

    // Выделяем память для массива указателей на чанки
    unsigned char** chunks =
        (unsigned char**)malloc(*num_chunks * sizeof(unsigned char*));
    if (chunks == NULL) {
        // Ошибка выделения памяти
        return NULL;
    }

    // Разбиваем буфер на чанки
    size_t offset = 0;
    for (size_t i = 0; i < *num_chunks; i++) {
        // Выделяем память для каждого чанка
        chunks[i] = (unsigned char*)malloc(chunk_size);
        if (chunks[i] == NULL) {
            // Ошибка выделения памяти,
            // освобождаем ранее выделенную память
            for (size_t j = 0; j < i; j++) {
                free(chunks[j]);
            }
            free(chunks);
            return NULL;
        }

        // Определяем количество байт для копирования
        size_t bytes_to_copy = chunk_size;
        if (offset + bytes_to_copy > buffer_length) {
            bytes_to_copy = buffer_length - offset;
        }

        // Копируем данные в чанк
        memcpy(chunks[i], buffer + offset, bytes_to_copy);

        // Дополняем нулями, если последний чанк неполный
        if (bytes_to_copy < chunk_size) {
            memset(chunks[i] + bytes_to_copy, 0,
                   chunk_size - bytes_to_copy);
        }

        offset += bytes_to_copy;
    }

    return chunks;
}

unsigned char* encrypt_chunk(const unsigned char* chunk,
                             size_t chunk_len,
                             EVP_PKEY* public_key,
                             size_t* encrypted_len) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, NULL);
    if (!ctx) {
        LOG_TXT("Error creating context for encryption");
        return NULL;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error initializing encryption");
        return NULL;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx,
                                     RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error setting RSA padding");
        return NULL;
    }

    size_t encrypted_length = 0;

    if (EVP_PKEY_encrypt(ctx, NULL, &encrypted_length, chunk,
                         chunk_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error determining buffer length for encryption");
        return NULL;
    }

    unsigned char* encrypted =
        (unsigned char*)malloc(encrypted_length);
    if (!encrypted) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error allocating memory for encryption");
        return NULL;
    }

    if (EVP_PKEY_encrypt(ctx, encrypted, &encrypted_length, chunk,
                         chunk_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        free(encrypted);
        LOG_TXT("Error encrypting message");
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);
    *encrypted_len = encrypted_length;
    return encrypted;
}


unsigned char* encipher(EVP_PKEY* private_key,
                        EVP_PKEY* public_key,
                        const char* msg,
                        int msg_len,
                        size_t* out_len)
{
    printf("msg: %s\n", msg);
    LOG_HEX("msg", msg, msg_len);

    // rnd_arr - array of unsigned char of random length
    // from 0 to 255 and random content, which is
    // necessary to make it impossible to guess the
    // meaning of the message by its length (for
    // example, for short messages like "yes" or "no").
    srand(time(NULL));
    uint8_t rnd_size = rand() % 256;
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
    LOG_HEX("rnd_size", &rnd_size, 1);
    /* LOG_HEX("rnd_arr", rnd_arr, rnd_size); */
    // now we have rnd_size and rnd_arr

    // Calculate msg_size in bytes
    uint16_t msg_size = msg_len;
    if (msg_size != msg_len) {
        perror("Err: msg too long");
        exit(-1);
    }
    LOG_HEX("msg_size", &msg_size, 2);

    // Calculate msg_crc
    unsigned char *msg_crc =
        malloc(HASH_SIZE * sizeof(unsigned char));
    if (-1 == calc_crc(msg, msg_len, msg_crc)) {
        perror("Err: bad crc");
        exit(-1);
    }
    LOG_HEX("msc_crc", msg_crc, HASH_SIZE);

    // Calculate msg_sign
    size_t sign_len = 0;
    unsigned char *msg_sign = NULL;
    if (-1 == calc_sign(msg, private_key, &sign_len, &msg_sign)) {
        perror("Err: bad sign");
        exit(-1);
    }
    /* LOG_HEX("msc_sign", msg_sign, SIG_SIZE); */

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
    memcpy(env, &(rnd_size), 1);
    memcpy(env+1, rnd_arr, rnd_size);
    memcpy(env+1+rnd_size, &msg_size, 2);
    memcpy(env+1+rnd_size+2, msg_crc, HASH_SIZE);
    memcpy(env+1+rnd_size+2+HASH_SIZE, msg_sign, SIG_SIZE);
    memcpy(env+1+rnd_size+2+HASH_SIZE+SIG_SIZE, msg, msg_size);

    // Split to chunks
    size_t num_chunks = 0;
    unsigned char** chunks =
        split_buffer(env, env_size, CHUNK_SIZE, &num_chunks);
    if (chunks == NULL) {
        fprintf(stderr, "Err: split chunks\n");
        exit(-1);
    }

    // Allocate enc_chunks
    unsigned char** enc_chunks =
        malloc(num_chunks * sizeof(unsigned char*));
    if (enc_chunks == NULL) {
        fprintf(stderr, "Err: memory allocation for enc_chunks\n");
        exit(-1);
    }

    // Encrypt each chunk
    for (size_t i = 0; i < num_chunks; i++) {
        size_t encrypted_len = 0;
        enc_chunks[i] =
            encrypt_chunk(chunks[i], CHUNK_SIZE,
                          public_key, &encrypted_len);
        if (enc_chunks[i] == NULL) {
            fprintf(stderr,
                    "Err: encryption failed for chunk %zu\n", i);
            // Free previously allocated encrypted chunks
            for (size_t j = 0; j < i; j++) {
                free(enc_chunks[j]);
            }
            free(enc_chunks);
            exit(-1);
        }
        if (encrypted_len != ENC_CHUNK_SIZE) {
            fprintf(stderr,
                    "Err: unexpected encrypted chunk size %zu\n", i);
            // Free all encrypted chunks
            for (size_t j = 0; j <= i; j++) {
                free(enc_chunks[j]);
            }
            free(enc_chunks);
            exit(-1);
        }
    }

    // Allocate pack buffer
    unsigned char *pack =
        malloc(2 + num_chunks * ENC_CHUNK_SIZE);

    // Save count of enc_chunks to pack
    uint16_t pack_size = num_chunks;
    /* LOG_HEX("pack_size", &pack_size, 2); */

    if (pack_size != num_chunks) {
        perror("Err: num_chunks too long");
        exit(-1);
    }
    memcpy(pack, &pack_size, 2);

    // Copy the encrypted chunks into pack
    for (size_t i = 0; i < num_chunks; i++) {
        memcpy(pack + 2 + i * ENC_CHUNK_SIZE, enc_chunks[i],
               ENC_CHUNK_SIZE);
    }
    /* LOG_HEX("pack", pack, 2 + pack_size * ENC_CHUNK_SIZE); */

    // Clean up allocated memory
    free(rnd_arr);
    free(msg_crc);
    OPENSSL_free(msg_sign);
    free(env);
    for (size_t i = 0; i < num_chunks; i++) {
        free(chunks[i]);
        free(enc_chunks[i]);
    }
    free(chunks);
    free(enc_chunks);

    *out_len = 2 + num_chunks * ENC_CHUNK_SIZE;
    return pack;
}


unsigned char* decrypt_chunk(const unsigned char* encrypted_chunk,
                             size_t encrypted_len,
                             EVP_PKEY* private_key,
                             size_t* decrypted_len) {
    size_t outlen = 0;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, NULL);
    if (!ctx) {
        LOG_TXT("Error creating context for decryption");
        return NULL;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error initializing decryption");
        return NULL;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx,
                                     RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error setting RSA padding");
        return NULL;
    }

    if (EVP_PKEY_decrypt(ctx, NULL, &outlen, encrypted_chunk,
                         encrypted_len) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error determining buffer length for decryption");
        return NULL;
    }

    unsigned char* decrypted =
        (unsigned char*)malloc(outlen);
    if (!decrypted) {
        EVP_PKEY_CTX_free(ctx);
        LOG_TXT("Error allocating memory for decryption");
        return NULL;
    }

    if (EVP_PKEY_decrypt(ctx, decrypted, &outlen, encrypted_chunk,
                         encrypted_len) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        free(decrypted);
        LOG_TXT("Error decrypting message");
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);
    *decrypted_len = outlen;
    return decrypted;
}

int verify_sign(const char* msg, size_t msg_len,
                const unsigned char* sign, size_t sign_len,
                EVP_PKEY* key)
{
    EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
    if (mdctx == NULL) {
        fprintf(stderr, "Err: Verify: Failed to create EVP_MD_CTX\n");
        return -1;
    }

    if (1 != EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, key)) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr, "Err: Verify: initializing verify: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    if (1 != EVP_DigestVerifyUpdate(mdctx, msg, msg_len)) {
        EVP_MD_CTX_free(mdctx);
        fprintf(stderr, "Err: Verify: updating verify: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    int ret = EVP_DigestVerifyFinal(mdctx, sign, sign_len);
    EVP_MD_CTX_free(mdctx);

    if (ret != 1) {
        if (ret == 0) {
            fprintf(stderr,
                    "Err: Verify: signature verification failed\n");
        } else {
            fprintf(stderr, "Err: Verify: verifying signature: %s\n",
                    ERR_error_string(ERR_get_error(), NULL));
        }
        return -1;
    }

    return 1; // Signature verification successful
}


unsigned char* decipher(EVP_PKEY* private_key, EVP_PKEY* public_key,
                        const unsigned char* pack, size_t in_len) {

    /* LOG_HEX("pack", pack, in_len); */

    // Get pack_size (in enc_chunks)
    uint16_t pack_size;
    memcpy(&pack_size, pack, 2);
    /* LOG_HEX("pack_size", (uint8_t*)&pack_size, 2); */

    // Allocate array for decrypted chunks
    unsigned char** decrypted_chunks =
        malloc(pack_size * sizeof(unsigned char*) * ENC_CHUNK_SIZE);
    if (!decrypted_chunks) {
        fprintf(stderr, "Err: memory allocation for decrypted chunks\n");
        return NULL;
    }

    // Decrypting chunks
    size_t total_decrypted_len = 0;
    for (size_t i = 0; i < pack_size; i++) {
        const unsigned char* enc_chunk_ptr =
            pack + 2 + i * ENC_CHUNK_SIZE;
        size_t dec_chunk_len = 0;
        unsigned char* dec_chunk =
            decrypt_chunk(enc_chunk_ptr, ENC_CHUNK_SIZE,
                          private_key, &dec_chunk_len);
        if (!dec_chunk) {
            fprintf(stderr,
                    "Err: decryption failed for chunk %zu\n", i);
            // Clean up
            for (size_t j = 0; j < i; j++) {
                free(decrypted_chunks[j]);
            }
            free(decrypted_chunks);
            return NULL;
        }
        if (dec_chunk_len != CHUNK_SIZE) {
            fprintf(stderr,
                    "Err: decrypted chunk size at chunk %zu\n",
                    i);
            // Clean up
            for (size_t j = 0; j <= i; j++) {
                free(decrypted_chunks[j]);
            }
            free(decrypted_chunks);
            return NULL;
        }
        decrypted_chunks[i] = dec_chunk;
        total_decrypted_len += dec_chunk_len;
    }

    // Merge decrypted chunks into env
    unsigned char* env = malloc(total_decrypted_len);
    if (!env) {
        fprintf(stderr, "Err: memory allocation for env\n");
        // Clean up
        for (size_t i = 0; i < pack_size; i++) {
            free(decrypted_chunks[i]);
        }
        free(decrypted_chunks);
        return NULL;
    }
    for (size_t i = 0; i < pack_size; i++) {
        memcpy(env + i * CHUNK_SIZE, decrypted_chunks[i], CHUNK_SIZE);
        free(decrypted_chunks[i]);
    }
    free(decrypted_chunks);


    // Now parse env to extract rnd_size and rnd_arr
    size_t offset = 0;
    if (total_decrypted_len < 1) {
        fprintf(stderr, "Err: env too small\n");
        free(env);
        return NULL;
    }
    uint8_t rnd_size = env[offset];
    offset += 1;

    LOG_HEX("rnd_size", &rnd_size, 1);

    if (rnd_size == 0) {
        fprintf(stderr, "Err: invalid rnd_size\n");
        free(env);
        return NULL;
    }
    if (offset + rnd_size > total_decrypted_len) {
        fprintf(stderr, "Err: env too small for rnd_arr\n");
        free(env);
        return NULL;
    }
    // Skipping rnd_arr
    offset += rnd_size;


    // msg_size
    if (offset + 2 > total_decrypted_len) {
        fprintf(stderr, "Err: env too small for msg_size\n");
        free(env);
        return NULL;
    }
    uint16_t msg_size;
    memcpy(&msg_size, env + offset, 2);
    offset += 2;

    LOG_HEX("msg_size", &msg_size, 2);

    // msg_crc
    if (offset + HASH_SIZE > total_decrypted_len) {
        fprintf(stderr, "Err: env too small for msg_crc\n");
        free(env);
        return NULL;
    }
    unsigned char* msg_crc = env + offset;
    offset += HASH_SIZE;

    LOG_HEX("msg_crc", msg_crc, HASH_SIZE);

    // msg_sign
    if (offset + SIG_SIZE > total_decrypted_len) {
        fprintf(stderr, "Err: env too small for msg_sign\n");
        free(env);
        return NULL;
    }
    unsigned char* msg_sign = env + offset;
    offset += SIG_SIZE;

    /* LOG_HEX("msg_sign", msg_sign, SIG_SIZE); */

    // msg
    if (offset + msg_size > total_decrypted_len) {
        fprintf(stderr, "Err: env too small for msg\n");
        free(env);
        return NULL;
    }
    unsigned char* msg = env + offset;

    LOG_HEX("msg", msg, msg_size);
    printf("msg: %s\n", msg);

    // Now, verify msg_crc
    unsigned char computed_crc[HASH_SIZE];
    if (calc_crc((const char*)msg, msg_size, computed_crc) == -1) {
        fprintf(stderr, "Err: calc_crc failed\n");
        free(env);
        return NULL;
    }
    if (memcmp(computed_crc, msg_crc, HASH_SIZE) != 0) {
        fprintf(stderr, "Err: CRC mismatch\n");
        free(env);
        return NULL;
    }

    // Now, verify msg_sign
    if (verify_sign((const char*)msg, msg_size, msg_sign,
                    SIG_SIZE, public_key) != 1) {
        fprintf(stderr, "Err: signature verification failed\n");
        free(env);
        return NULL;
    }

    // Copy msg to a new buffer to return
    unsigned char* msg_ret = malloc(msg_size);
    if (!msg_ret) {
        fprintf(stderr, "Err: memory allocation for msg_ret\n");
        free(env);
        return NULL;
    }
    memcpy(msg_ret, msg, msg_size);
    /* *msg_len_out = msg_size; */

    /* // Clean up */
    free(env);
    return msg_ret;
}



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
        fprintf(stderr, "Error loading key: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        return NULL;
    }

    return key;
}
