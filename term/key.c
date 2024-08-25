// key.c

#include "key.h"

#include "all_libs.h"

// Создание массива строковых представлений key_strings[]
// для enum Key
#define GENERATE_STRING(val, len, str, is_printable) str,
const char* key_strings[] = {
    KEY_MAP(GENERATE_STRING)
};

// Создание массива длин, индексы в котором соответствуют
// индексам в массиве key_strings
#define GENERATE_LENGTH(val, len, str, is_printable) len,
const int key_lengths[] = {
    KEY_MAP(GENERATE_LENGTH)
};

// Создание функции key_to_string() для преобразования
// Key в соответствующую ему строку
#define GENERATE_KEY_STR(val, len, str, is_printable) #val,
const char* key_to_str(Key key) {
    static const char* key_names[] = {
        KEY_MAP(GENERATE_KEY_STR)
    };
    return key_names[key];
}

Key identify_key(const char* seq, int seq_size) {
    for (int i = 0; i < sizeof(key_strings) / sizeof(key_strings[0]); i++) {
        if ( (key_lengths[i] == seq_size) &&
             (strncmp(seq, key_strings[i], key_lengths[i]) == 0) ) {
            return (Key)i;
        }
    }
    return KEY_UNKNOWN; // return KEY_UNKNOWN if seq not found
}
