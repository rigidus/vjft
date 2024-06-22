// test_crypto.hpp
#ifndef TEST_CRYPTO_HPP
#define TEST_CRYPTO_HPP

#include "defs.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "Client.hpp"
#include "Message.hpp"
#include "Crypt.hpp"
#include "Utils.hpp"

#define CHUNK_SIZE 255
#define ENC_CHUNK_SIZE 512

// Функция для тестирования полной последовательности шифрования,
// кодирования, подписывания и обратного процесса
bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key);

#endif // TEST_CRYPTO_HPP
