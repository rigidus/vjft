// cmd.c

#include "cmd.h"

#include "utf8.h"

extern int sockfd;

void cmd_connect(MessageNode* msg, const char* param) {
    connect_to_server("127.0.0.1", 8888);
}

void cmd_alt_enter(MessageNode* msg, const char* param) {
    /* pushMessage(&messageList, "ALT enter function"); */
    if (!msg || !msg->message) return;  // Проверяем, что msg и msg->message не NULL

    // Вычисляем byte_offset, используя текущую позицию курсора
    int byte_offset = utf8_byte_offset(msg->message, msg->cursor_pos);

    // Выделяем новую память для сообщения
    char* new_message = malloc(strlen(msg->message) + 2);  // +2 для новой строки и нуль-терминатора
    if (!new_message) {
        perror("Failed to allocate memory for new message");
        return;
    }

    // Копируем часть сообщения до позиции курсора
    strncpy(new_message, msg->message, byte_offset);
    new_message[byte_offset] = '\n';  // Вставляем символ новой строки

    // Копируем оставшуюся часть сообщения
    strcpy(new_message + byte_offset + 1, msg->message + byte_offset);

    // Освобождаем старое сообщение и обновляем указатель
    free(msg->message);
    msg->message = new_message;

    msg->cursor_pos++; // Перемещаем курсор на следующий символ после '\n'

    // При необходимости обновляем UI или логику отображения
    // ...
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

void cmd_backspace(MessageNode* msg, const char* param) {
    if (!msg || !msg->message || msg->cursor_pos == 0) return;

    // Находим байтовую позицию текущего и предыдущего UTF-8 символов
    int byte_offset_current = utf8_byte_offset(msg->message, msg->cursor_pos);
    int new_cursor_pos = msg->cursor_pos-1;
    if (new_cursor_pos < 0) {
        new_cursor_pos = 0;
    }
    int byte_offset_prev = utf8_byte_offset(msg->message, new_cursor_pos);

    // Вычисляем новую длину сообщения
    int new_length = strlen(msg->message) - (byte_offset_current - byte_offset_prev) + 1;
    char* new_message = malloc(new_length);
    if (!new_message) {
        perror("Не удалось выделить память");
        return;
    }

    // Копирование части строки до предыдущего символа
    strncpy(new_message, msg->message, byte_offset_prev);

    // Копирование части строки после текущего символа
    strcpy(new_message + byte_offset_prev, msg->message + byte_offset_current);

    // Освобождаем старое сообщение и присваиваем новое
    free(msg->message);
    msg->message = new_message;
    msg->cursor_pos = new_cursor_pos; // Обновляем позицию курсора
}

void cmd_backward_char(MessageNode* node, const char* stub) {
    if (node->cursor_pos > 0) { node->cursor_pos--; }
}

void cmd_forward_char(MessageNode* node, const char* stub) {
    // >=, чтобы позволить курсору стоять на позиции нулевого символа
    int len = utf8_strlen(node->message);
    if (++node->cursor_pos >= len) { node->cursor_pos = len; }
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


/* Clipboard commands  */

typedef struct ClipboardNode {
    char* text;
    struct ClipboardNode* next;
} ClipboardNode;

ClipboardNode* clipboard_head = NULL;

void push_clipboard(const char* text) {
    ClipboardNode* new_node = malloc(sizeof(ClipboardNode));
    if (new_node == NULL) return; // Обработка ошибки выделения памяти
    new_node->text = strdup(text);
    new_node->next = clipboard_head;
    clipboard_head = new_node;
}

char* pop_clipboard() {
    if (clipboard_head == NULL) return NULL;
    ClipboardNode* top_node = clipboard_head;
    clipboard_head = clipboard_head->next;
    char* text = top_node->text;
    free(top_node);
    return text;
}

void cmd_copy(MessageNode* node, const char* param) {
    int start = min(node->cursor_pos, node->shadow_cursor_pos);
    int end = max(node->cursor_pos, node->shadow_cursor_pos);

    int start_byte = utf8_byte_offset(node->message, start);
    int end_byte = utf8_byte_offset(node->message, end);

    if (start_byte != end_byte) {
        char* text_to_copy = strndup(node->message + start_byte, end_byte - start_byte);
        push_clipboard(text_to_copy);
        free(text_to_copy);
    }
}

void cmd_cut(MessageNode* node, const char* param) {
    cmd_copy(node, param); // Сначала копируем текст

    int start = min(node->cursor_pos, node->shadow_cursor_pos);
    int end = max(node->cursor_pos, node->shadow_cursor_pos);

    int start_byte = utf8_byte_offset(node->message, start);
    int end_byte = utf8_byte_offset(node->message, end);

    memmove(node->message + start_byte, node->message + end_byte,
            strlen(node->message) - end_byte + 1);
    node->cursor_pos = start;
    node->shadow_cursor_pos = start;
}

void cmd_paste(MessageNode* node, const char* param) {
    char* text_to_paste = pop_clipboard();
    if (text_to_paste) {
        cmd_insert(node, text_to_paste);
        free(text_to_paste);
    }
}

void cmd_toggle_cursor_shadow(MessageNode* node, const char* param) {
    int temp = node->cursor_pos;
    node->cursor_pos = node->shadow_cursor_pos;
    node->shadow_cursor_pos = temp;
}


void cmd_undo(MessageNode* msg, const char* param) {
    undoLastEvent();  // Вызов функции отмены последнего действия
}

void cmd_redo(MessageNode* msg, const char* param) {
    redoLastEvent();  // Вызов функции повтора последнего действия
}
