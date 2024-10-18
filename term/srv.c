#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define PORT 8888            // Порт, на котором сервер будет слушать
#define MAX_CLIENTS 10       // Максимальное количество клиентов
#define BUFFER_SIZE 1024     // Размер буфера для приема данных

int main() {
    int listener_fd, new_fd;           // Сокет для прослушивания и новый сокет для клиента
    struct sockaddr_in server_addr;     // Структура адреса сервера
    struct sockaddr_in client_addr;     // Структура адреса клиента
    socklen_t addrlen;
    char buffer[BUFFER_SIZE];
    int i, j, nbytes;

    // Массив для хранения клиентских сокетов
    int client_fds[MAX_CLIENTS];
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }

    // Создание сокета
    if ((listener_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Установка параметров сокета (опции)
    int yes = 1;
    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        close(listener_fd);
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Привязка к любому доступному интерфейсу
    server_addr.sin_port = htons(PORT);
    memset(&(server_addr.sin_zero), '\0', 8); // Заполнение нулями

    // Привязка сокета к адресу
    if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listener_fd);
        exit(EXIT_FAILURE);
    }

    // Начало прослушивания
    if (listen(listener_fd, 10) == -1) {
        perror("listen");
        close(listener_fd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на порту %d. Ожидание подключений...\n", PORT);

    // Основной цикл сервера
    while (1) {
        fd_set master_set, read_fds;
        int fd_max = listener_fd;

        FD_ZERO(&master_set);
        FD_SET(listener_fd, &master_set);

        // Добавление клиентских сокетов в master_set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &master_set);
                if (client_fds[i] > fd_max) {
                    fd_max = client_fds[i];
                }
            }
        }

        read_fds = master_set;

        // Использование select для ожидания активности
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Обработка событий
        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener_fd) {
                    // Новое входящее соединение
                    addrlen = sizeof(client_addr);
                    if ((new_fd = accept(listener_fd, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
                        perror("accept");
                        continue;
                    }

                    // Добавление нового клиента в массив
                    for (j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] == -1) {
                            client_fds[j] = new_fd;
                            printf("Новый клиент подключен: %s:%d (Сокет FD: %d)\n",
                                   inet_ntoa(client_addr.sin_addr),
                                   ntohs(client_addr.sin_port),
                                   new_fd);
                            break;
                        }
                    }

                    if (j == MAX_CLIENTS) {
                        printf("Максимальное количество клиентов достигнуто. Отказ в подключении.\n");
                        close(new_fd);
                    }
                } else {
                    // Данные от существующего клиента
                    memset(buffer, 0, sizeof(buffer));
                    if ((nbytes = recv(i, buffer, sizeof(buffer) - 1, 0)) <= 0) {
                        if (nbytes == 0) {
                            // Соединение закрыто клиентом
                            printf("Сокет FD %d отключился.\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        // Удаление клиента из массива
                        for (j = 0; j < MAX_CLIENTS; j++) {
                            if (client_fds[j] == i) {
                                client_fds[j] = -1;
                                break;
                            }
                        }
                    } else {
                        // Получено сообщение
                        printf("Сообщение от FD %d: %s\n", i, buffer);
                    }
                }
            }
        }
    }

    // Закрытие сокета (достигается только при выходе из цикла)
    close(listener_fd);
    return 0;
}
