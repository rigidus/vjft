// mbuf.c

#include "mbuf.h"

// Глобальный минибуфер
MiniBuffer miniBuffer = {NULL, 0, 0};

// Функция для инициализации минибуфера
void initMiniBuffer(size_t initial_capacity) {
    miniBuffer.buffer = malloc(initial_capacity);
    if (miniBuffer.buffer == NULL) {
        fprintf(stderr, "Не удалось выделить память для минибуфера\n");
        exit(EXIT_FAILURE);
    }
    miniBuffer.buffer[0] = '\0'; // Начальное состояние буфера
    miniBuffer.size = 0;
    miniBuffer.capacity = initial_capacity;
}

// Функция для освобождения минибуфера
void freeMiniBuffer() {
    free(miniBuffer.buffer);
    miniBuffer.buffer = NULL;
    miniBuffer.size = 0;
    miniBuffer.capacity = 0;
}

// Функция для добавления текста в минибуфер
void appendToMiniBuffer(const char* text) {
    if (text == NULL) return; // Проверка на NULL

    size_t text_len = strlen(text);
    size_t new_size = miniBuffer.size + text_len;

    // Проверяем, нужно ли расширить буфер
    if (new_size >= miniBuffer.capacity) {
        size_t new_capacity
            = miniBuffer.capacity == 0 ? 128 : miniBuffer.capacity * 2;
        while (new_capacity <= new_size) {
            new_capacity *= 2;
        }
        char* new_buffer = realloc(miniBuffer.buffer, new_capacity);
        if (new_buffer == NULL) {
            fprintf(stderr, "Realloc error in appendToMiniBuffer\n");
            freeMiniBuffer();
            exit(EXIT_FAILURE);
        }
        miniBuffer.buffer = new_buffer;
        miniBuffer.capacity = new_capacity;
    }

    // Добавляем текст в конец буфера
    strcat(miniBuffer.buffer + miniBuffer.size, text);
    miniBuffer.size += text_len;
}

// Функция для очистки минибуфера
void clearMiniBuffer() {
    if (miniBuffer.buffer) {
        miniBuffer.buffer[0] = '\0'; // Сбрасываем содержимое буфера
        miniBuffer.size = 0;
    }
}
