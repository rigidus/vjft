// key.h

#ifndef KEY_HEADER /* don't confuse it with a KEY_H !!! */
#define KEY_HEADER

#include "key_map.h"

// Создание enum с помощью макроса
#define GENERATE_ENUM(val, len, str, is_printable) val,
typedef enum {
    KEY_MAP(GENERATE_ENUM)
} Key;

/* extern const char* key_strings[]; */
extern const int key_lengths[];
extern const char* key_to_str(Key key);

Key identify_key(const char* seq, int seq_size);

#endif
