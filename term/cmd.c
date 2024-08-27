// cmd.c

#include "cmd.h"

#include "utf8.h"

extern int sockfd;

void cmd_connect(MessageNode* msg, const char* param) {
    connect_to_server("127.0.0.1", 8888);
}

void cmd_alt_enter(MessageNode* msg, const char* param) {
    pushMessage(&messageList, "ALT enter function");
}

void cmd_enter(MessageNode* msg, const char* param) {
    pushMessage(&messageList, "enter function");
    if (messageList.current == NULL) {
        pushMessage(&messageList, "cmd_enter_err: Нет сообщения для отправки");
        return;
    }
    const char* text = msg->message;
    if (text == NULL || strlen(text) == 0) {
        pushMessage(&messageList, "cmd_enter_err: Текущее сообщение пусто");
        return;
    }
    if (sockfd <= 0) {
        pushMessage(&messageList, "cmd_enter_err: Нет соединения");
        return;
    }

    ssize_t bytes_sent = send(sockfd, text, strlen(text), 0);
    if (bytes_sent < 0) {
        pushMessage(&messageList, "cmd_enter_err: Ошибка при отправке сообщения");
    } else {
        pushMessage(&messageList, "cmd_enter: отправлено успешно");
    }

}

void cmd_backward_char(MessageNode* node, const char* stub) {
    if (node->cursor_pos > 0) { node->cursor_pos--; }
}

void cmd_forward_char(MessageNode* node, const char* stub) {
    int len = utf8_strlen(node->message);
    if (++node->cursor_pos > len) { node->cursor_pos = len; }
}


// Перемещение курсора вперед на одно слово
void cmd_forward_word(MessageNode* node, const char* stub) {
    int len = utf8_strlen(node->message);  // Длина строки в байтах

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Пропуск пробелов (если курсор находится на пробеле)
    while (byte_offset < len && isspace((unsigned char)node->message[byte_offset]))
    {
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }

    // Перемещение вперед до конца слова
    while (byte_offset < len && !isspace((unsigned char)node->message[byte_offset]))
    {
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }
}

// Перемещение курсора назад на одно слово
void cmd_backward_word(MessageNode* node, const char* stub) {
    if (node->cursor_pos == 0) return;  // Если курсор уже в начале, ничего не делаем

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Пропуск пробелов, если курсор находится на пробеле
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->message, byte_offset);
        if (!isspace((unsigned char)node->message[prev_offset])) {
            break;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }

    // Перемещение назад до начала предыдущего слова
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->message, byte_offset);
        if (isspace((unsigned char)node->message[prev_offset])) {
            break;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }
}

void cmd_to_beginning_of_line(MessageNode* node, const char* stub) {
    // Если курсор уже в начале текста, ничего не делаем
    if (node->cursor_pos == 0) return;

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Движемся назад, пока не найдем начало строки или начало текста
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->message, byte_offset);
        if (node->message[prev_offset] == '\n') {
            // Переходим на следующий символ после \n
            byte_offset = utf8_next_char(node->message, prev_offset);
            node->cursor_pos = utf8_strlen(node->message);
            return;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }
    // Если достигли начала текста, устанавливаем курсор на позицию 0
    node->cursor_pos = 0;
}

void cmd_to_end_of_line(MessageNode* node, const char* stub) {
    // Длина строки в байтах
    int len = strlen(node->message);

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Движемся вперед, пока не найдем конец строки или конец текста
    while (byte_offset < len) {
        if (node->message[byte_offset] == '\n') {
            node->cursor_pos = utf8_char_index(node->message, byte_offset);
            return;
        }
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }

    // Если достигли конца текста
    node->cursor_pos = utf8_char_index(node->message, len);
}

void cmd_prev_msg() {
    moveToPreviousMessage(&messageList);
}

void cmd_next_msg() {
    moveToNextMessage(&messageList);
}



// Функция для вставки текста в позицию курсора
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void cmd_insert(MessageNode* node, const char* insert_text) {
    pthread_mutex_lock(&lock);
    if (!node || !node->message) return;

    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);
    int insert_len = strlen(insert_text);

    char* new_message = realloc(node->message, strlen(node->message) + insert_len + 1);
    if (new_message == NULL) {
        perror("Failed to reallocate memory");
        exit(1);
    }
    node->message = new_message;

    memmove(node->message + byte_offset + insert_len,
            node->message + byte_offset,
            strlen(node->message) - byte_offset + 1);
    memcpy(node->message + byte_offset,
           insert_text,
           insert_len);

    node->cursor_pos += utf8_strlen(insert_text);
    pthread_mutex_unlock(&lock);
}
