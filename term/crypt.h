#ifndef CRYPT_H
#define CRYPT_H

#include "all_libs.h"

#define DBG 0
#define DBG_MSG 0
#define DBG_CRYPT 0

#define SIG_SIZE 512
#define HASH_SIZE 32
#define CHUNK_SIZE 255
#define ENC_CHUNK_SIZE 512
// Минимальная длина пакета:
// - 2 байта длины
// - Envelope (1536 байт) :
//   - 1 байт random_len
//   - 0 байт random
//   - 2 байта msg_size
//   - 32 байта msg_crc
//   - 512 байта msg_sign
//   - 1 байт msg
//   Всего 548 байт, это 3 незашифрованных чанка по 255 байт
//   После шифрования чанки будут по 512 байт, значит 1536 байт
// - 32 байта Sync marker
// = 1570 байт
#define MIN_PACK_SIZE 1570
// Максимальная длина пакета:
// - 32 чанка по 512 байт в Envelope = 16385
// - 2 байта длины
// - 32 байта Sync marker
// = 16418
#define MAX_MSG_SIZE 16418
#define MAX_PACK_SIZE 16384
#define READ_QUEUE_SIZE 32
#define SYNC_MARKER_SIZE 32


EVP_PKEY* load_key_from_file(const char* key_file, int is_private, const char* password);

int calc_crc(const char* msg, size_t msg_len,
             unsigned char* hash);

int calc_sign(const char* msg, EVP_PKEY* key,
              size_t* sign_len, unsigned char** sign_ret);


unsigned char* encipher(EVP_PKEY* private_key,
                        EVP_PKEY* public_key,
                        const char* msg,
                        int msg_len,
                        size_t* out_len);

unsigned char* decipher(EVP_PKEY* private_key,
                        EVP_PKEY* public_key,
                        const unsigned char* pack,
                        size_t* msg_len_out);

#endif // CRYPT_H
