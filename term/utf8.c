// utf8.c

#include "utf8.h"

// Функция для вычисления длины UTF-8 символа
size_t utf8_char_length(const char* c) {
    unsigned char byte = (unsigned char)*c;
    if (byte <= 0x7F) return 1;        // ASCII
    else if ((byte & 0xE0) == 0xC0) return 2; // 2-byte sequence
    else if ((byte & 0xF0) == 0xE0) return 3; // 3-byte sequence
    else if ((byte & 0xF8) == 0xF0) return 4; // 4-byte sequence
    return 1; // Ошибочные символы обрабатываем как 1 байт
}

// Возвращает длину строки в utf-8 символах
size_t utf8_strlen(const char *str) {
    size_t length = 0;
    for (; *str; length++) {
        str += utf8_char_length(str);
    }
    return length;
}

// Функция для перемещения курсора на следующий UTF-8 символ
int utf8_next_char(const char* str, int pos) {
    if (str[pos] == '\0') return pos;
    return pos + utf8_char_length(&str[pos]);
}

// Функция для перемещения курсора на предыдущий UTF-8 символ
int utf8_prev_char(const char* str, int pos) {
    if (pos == 0) return 0;
    do {
        pos--;
    } while (pos > 0 && ((str[pos] & 0xC0) == 0x80)); // Пропуск байтов продолжения
    return pos;
}

// Функция для определения смещения в байтах
// для указанной позиции курсора (в символах)
int utf8_byte_offset(const char* str, int cursor_pos) {
    int byte_offset = 0;
    for (int i = 0; i < cursor_pos && str[byte_offset]; i++) {
        byte_offset += utf8_char_length(str + byte_offset);
    }
    return byte_offset;
}

// Функция для конвертации byte_offset в индекс символа (UTF-8)
int utf8_char_index(const char* str, int byte_offset) {
    int char_index = 0;
    int current_offset = 0;

    while (current_offset < byte_offset && str[current_offset] != '\0') {
        int char_len = utf8_char_length(&str[current_offset]);
        current_offset += char_len;
        char_index++;
    }

    return char_index;
}

// Проверка, что UTF-8 символ полностью считан
bool is_utf8_complete(const char* buffer, int len) {
    if (len == 0) return false;
    int expected_len = 0;
    unsigned char c = buffer[0];
    if ((c & 0x80) == 0) expected_len = 1;          // 0xxxxxxx
    else if ((c & 0xE0) == 0xC0) expected_len = 2;  // 110xxxxx
    else if ((c & 0xF0) == 0xE0) expected_len = 3;  // 1110xxxx
    else if ((c & 0xF8) == 0xF0) expected_len = 4;  // 11110xxx
    return expected_len == len;
}


// функция для декодирования символа UTF-8 в его кодовую точку Unicode
int utf8_decode(const char* s, size_t len, uint32_t* codepoint) {
    if (len == 0) return 0;
    const unsigned char *bytes = (const unsigned char*)s;
    uint32_t cp = 0;
    if (bytes[0] < 0x80) {
        cp = bytes[0];
        *codepoint = cp;
        return 1;
    } else if ((bytes[0] & 0xE0) == 0xC0) {
        if (len < 2) return 0;
        cp = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
        *codepoint = cp;
        return 2;
    } else if ((bytes[0] & 0xF0) == 0xE0) {
        if (len < 3) return 0;
        cp = ((bytes[0] & 0x0F) << 12) |
            ((bytes[1] & 0x3F) << 6) |
            (bytes[2] & 0x3F);
        *codepoint = cp;
        return 3;
    } else if ((bytes[0] & 0xF8) == 0xF0) {
        if (len < 4) return 0;
        cp = ((bytes[0] & 0x07) << 18) |
            ((bytes[1] & 0x3F) << 12) |
            ((bytes[2] & 0x3F) << 6) |
            (bytes[3] & 0x3F);
        *codepoint = cp;
        return 4;
    }
    return 0;
}

// Используя кодовую точку Unicode, мы можем определить, является
// ли символ пробелом или знаком пунктуации

bool is_codepoint_whitespace(uint32_t cp) {
    // Check for common whitespace characters
    return (cp == 0x20) || (cp >= 0x09 && cp <= 0x0D) || // ASCII whitespace
        (cp == 0x85) || (cp == 0xA0) ||               // Next line, Non-breaking space
        (cp >= 0x1680 && cp <= 0x200A) ||             // Various spaces
        (cp == 0x2028) || (cp == 0x2029) ||           // Line/Paragraph separator
        (cp == 0x202F) || (cp == 0x205F) || (cp == 0x3000); // Narrow no-break space, Medium mathematical space, Ideographic space
}

bool is_codepoint_punctuation(uint32_t cp) {
    // Check for common punctuation characters
    return (cp >= 0x21 && cp <= 0x2F) ||
        (cp >= 0x3A && cp <= 0x40) ||
        (cp >= 0x5B && cp <= 0x60) ||
        (cp >= 0x7B && cp <= 0x7E) ||
        (cp >= 0x2000 && cp <= 0x206F); // General punctuation
}

bool is_utf8_whitespace(const char* c, size_t len) {
    uint32_t cp;
    if (utf8_decode(c, len, &cp) == 0) return false;
    return is_codepoint_whitespace(cp);
}

bool is_utf8_punctuation(const char* c, size_t len) {
    uint32_t cp;
    if (utf8_decode(c, len, &cp) == 0) return false;
    return is_codepoint_punctuation(cp);
}
