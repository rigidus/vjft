// utf-8.h
#ifndef UTF8_H
#define UTF8_H

#include "all_libs.h"

// Функция для вычисления длины UTF-8 символа
size_t utf8_char_length(const char* c);

// Возвращает длину строки в utf-8 символах
size_t utf8_strlen(const char *str);

// Функция для перемещения курсора на следующий UTF-8 символ
int utf8_next_char(const char* str, int pos);

// Функция для перемещения курсора на предыдущий UTF-8 символ
int utf8_prev_char(const char* str, int pos);

// Функция для определения смещения в байтах
// для указанной позиции курсора (в символах)
int utf8_byte_offset(const char* str, int cursor_pos);

// Функция для конвертации byte_offset в индекс символа (UTF-8)
int utf8_char_index(const char* str, int byte_offset);

#endif
