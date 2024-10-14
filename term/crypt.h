#ifndef CRYPT_H
#define CRYPT_H

#include "all_libs.h"

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
                        size_t in_len);

EVP_PKEY* load_key_from_file(const char* key_file, int is_private,
                             const char* password);


#endif // CRYPT_H
