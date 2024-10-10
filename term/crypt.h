#ifndef CRYPT_H
#define CRYPT_H

#include "all_libs.h"

EVP_PKEY* load_key_from_file(const char* key_file, int is_private, const char* password);

unsigned char* encipher(EVP_PKEY* private_key, EVP_PKEY* public_key, const unsigned char* msg, size_t msg_len, size_t* out_len);

unsigned char* decipher(EVP_PKEY* private_key, EVP_PKEY* public_key, const unsigned char* encrypted_data, size_t encrypted_len, size_t* out_len);

// Не забудьте добавить другие необходимые функции из Crypt.cpp

#endif // CRYPT_H
