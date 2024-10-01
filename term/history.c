// history.c

#include "history.h"
#include "mbuf.h"

// Глобальный динамический буфер для истории событий
DynamicBuffer historyBuffer = {NULL, 0, 0};

// Инициализация динамического буфера
void initDynamicBuffer(DynamicBuffer* db, size_t initial_capacity) {
    db->buffer = malloc(initial_capacity);
    if (db->buffer == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(EXIT_FAILURE);
    }
    db->buffer[0] = '\0';
    db->size = 0;
    db->capacity = initial_capacity;
}

// Освобождение динамического буфера
void freeDynamicBuffer(DynamicBuffer* db) {
    free(db->buffer);
    db->buffer = NULL;
    db->size = 0;
    db->capacity = 0;
}

// Добавление текста в динамический буфер
void appendToDynamicBuffer(DynamicBuffer* db, const char* text) {
    if (text == NULL) return;
    size_t text_len = strlen(text);
    size_t new_size = db->size + text_len;
    if (new_size >= db->capacity) {
        size_t new_capacity = db->capacity == 0 ? 128 : db->capacity * 2;
        while (new_capacity <= new_size) {
            new_capacity *= 2;
        }
        char* new_buffer = realloc(db->buffer, new_capacity);
        if (new_buffer == NULL) {
            fprintf(stderr, "Ошибка перевыделения памяти\n");
            freeDynamicBuffer(db);
            exit(EXIT_FAILURE);
        }
        db->buffer = new_buffer;
        db->capacity = new_capacity;
    }
    strcat(db->buffer + db->size, text);
    db->size += text_len;
}

// Очистка динамического буфера
void clearDynamicBuffer(DynamicBuffer* db) {
    if (db->buffer) {
        db->buffer[0] = '\0';
        db->size = 0;
    }
}

// Инициализация истории событий
void initEventHistory(DynamicBuffer* db) {
    initDynamicBuffer(db, 512);  // Начальная вместимость
}

// Функция для отображения истории событий
void dispExEv(InputEvent* gHistoryEventQueue) {
    clearDynamicBuffer(&historyBuffer);
    appendToDynamicBuffer(&historyBuffer, "\nHistory: ");
    InputEvent* currentEvent = gHistoryEventQueue;
    while (currentEvent != NULL) {
        appendToDynamicBuffer(&historyBuffer, "(");
        appendToDynamicBuffer(&historyBuffer,
                              descr_cmd_fn(currentEvent->cmdFn));
        appendToDynamicBuffer(&historyBuffer, " ");
        appendToDynamicBuffer(
            &historyBuffer,
            currentEvent->seq ? currentEvent->seq : "NULL");
        appendToDynamicBuffer(&historyBuffer, ") ");
        currentEvent = currentEvent->next;
    }
    /* printf("%s\n", historyBuffer.buffer);  // Вывод истории событий */
    appendToMiniBuffer(historyBuffer.buffer);
}
