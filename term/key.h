// key.h

#include "key_map.h"

// Создание enum с помощью макроса
#define GENERATE_ENUM(val, len, str, is_printable) val,
typedef enum {
    KEY_MAP(GENERATE_ENUM)
} Key;
