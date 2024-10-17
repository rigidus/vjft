#ifndef CLIENT_H
#define CLIENT_H

#include "all_libs.h"

#define SYNC_SIZE 32

extern int sockfd;
extern int peer_count;

typedef struct {

    EVP_PKEY* private_key;
    EVP_PKEY** peer_public_keys; // Массив указателей на pubkeys
    size_t peer_count; // Количество pubkeys
} client_t;

// Инициализация клиента и подключение к серверу
int client_init(client_t* client, const char* host, int port,
                const char* priv_key_file, const char* password,
                const char** peer_pub_key_files, size_t peer_count);

// Отправка данных на сервер
int client_send(client_t* client, const char* data,
                size_t len);

// Получение данных с сервера
int client_receive(client_t* client,
                   unsigned char* buffer,
                   size_t max_len);

// Закрытие соединения
void client_close(client_t* client);

#endif // CLIENT_H
