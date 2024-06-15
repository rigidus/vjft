// test_crypto.hpp
#ifndef TEST_CRYPTO_HPP
#define TEST_CRYPTO_HPP

#include <iostream>
#include <vector>
#include <string>
#include <openssl/evp.h>
#include "Client.hpp"
#include "Message.hpp"
#include "Crypt.hpp"

// Функция для тестирования полной последовательности шифрования, кодирования, подписывания и обратного процесса
bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key);

#endif // TEST_CRYPTO_HPP
