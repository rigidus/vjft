#include "client.h"
#include "crypt.h"

int client_init(client_t* client,
                const char* host, int port,
                const char* priv_key_file,
                const char* password,
                const char** peer_pub_key_files,
                size_t peer_count) {
    client->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(client->sockfd,
                (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(client->sockfd);
        return -1;
    }

    client->private_key =
        load_key_from_file(priv_key_file, 1, password);
    if (!client->private_key) {
        close(client->sockfd);
        return -1;
    }

    client->peer_public_keys =
        malloc(peer_count * sizeof(EVP_PKEY*));
    if (!client->peer_public_keys) {
        EVP_PKEY_free(client->private_key);
        close(client->sockfd);
        return -1;
    }

    client->peer_count = peer_count;
    for (size_t i = 0; i < peer_count; ++i) {
        client->peer_public_keys[i] =
            load_key_from_file(peer_pub_key_files[i], 0, NULL);
        if (!client->peer_public_keys[i]) {
            for (size_t j = 0; j < i; ++j) {
                EVP_PKEY_free(client->peer_public_keys[j]);
            }
            free(client->peer_public_keys);
            EVP_PKEY_free(client->private_key);
            close(client->sockfd);
            return -1;
        }
    }

    return 0;
}

int client_send(client_t* client,
                const char* data,
                size_t len)
{
    // sync_marker : 32 нулевых байта
    uint8_t sync_marker[SYNC_SIZE] = {0};

    // Для каждого из ключей получателей..
    for (int i = 0; i < peer_count; ++i) {

        // Шифрование
        size_t out_len = 0;
        uint8_t *encrypted =
            encipher(client->private_key,
                     client->peer_public_keys[i],
                     data, len, &out_len);

        uint16_t packed_msg_size = 2 + out_len + SYNC_SIZE;

        uint8_t *packed_msg = malloc(packed_msg_size);

        memcpy(packed_msg, &packed_msg_size, 2);
        memcpy(packed_msg+2, encrypted, out_len);
        memcpy(packed_msg+2+out_len, &sync_marker, SYNC_SIZE);

        // Добавляем packed_msg в очередь сообщений на отправку
        /* write_msgs_.push_back(std::move(packed_msg)); */
        // Пока вместо просто отправляем
        ssize_t sent = send(client->sockfd, packed_msg, len, 0);
        if (sent < 0) {
            perror("send");
            return -1;
        }

    }
    return 0;
}

int client_receive(client_t* client, unsigned char* buffer, size_t max_len) {
    // Получаем данные (здесь вы можете реализовать буферизацию и обработку заголовков)
    ssize_t received = recv(client->sockfd, buffer, max_len, 0);
    if (received < 0) {
        perror("recv");
        return -1;
    }

    // Дешифруем данные
    /* std::vector<unsigned char> decrypted_data = decipher(client->private_key, client->peer_public_key, buffer, received); */

    // Копируем расшифрованные данные в буфер
    /* memcpy(buffer, decrypted_data.data(), decrypted_data.size()); */

    return received;
}

void client_close(client_t* client) {
    if (client->sockfd >= 0) {
        close(client->sockfd);
    }
    if (client->private_key) {
        EVP_PKEY_free(client->private_key);
    }
    for (size_t i = 0; i < client->peer_count; ++i) {
        if (client->peer_public_keys[i]) {
            EVP_PKEY_free(client->peer_public_keys[i]);
        }
    }
    free(client->peer_public_keys);
}
