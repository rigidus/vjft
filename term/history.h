// ctrlstk.h

#ifndef HISTORY_H
#define HISTORY_H

#include "all_libs.h"

#include "event.h"

// Структура для управления динамическим буфером
typedef struct {
    char* buffer;     // Указатель на буфер
    size_t size;      // Текущий размер буфера
    size_t capacity;  // Вместимость буфера
} DynamicBuffer;

extern DynamicBuffer historyBuffer;

void initDynamicBuffer(DynamicBuffer* db, size_t initial_capacity);
void freeDynamicBuffer(DynamicBuffer* db);
void appendToDynamicBuffer(DynamicBuffer* db, const char* text);
void clearDynamicBuffer(DynamicBuffer* db);
void initEventHistory(DynamicBuffer* db);
void dispExEv(InputEvent* gHistoryEventQueue);

#endif
