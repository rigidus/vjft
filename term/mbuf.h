// mbuf.h

#ifndef MBUF_H
#define MBUF_H

#include "all_libs.h"

// Структура для управления динамическим минибуфером
typedef struct {
    char* buffer;     // Указатель на буфер
    size_t size;      // Текущий размер буфера
    size_t capacity;  // Вместимость буфера
} MiniBuffer;


// Глобальный минибуфер
extern MiniBuffer miniBuffer;

void initMiniBuffer(size_t initial_capacity);
void freeMiniBuffer();
void appendToMiniBuffer(const char* text);
void clearMiniBuffer();

#endif
