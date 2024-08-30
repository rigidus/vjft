// kbd.h
#ifndef KBD_H
#define KBD_H

#include "all_libs.h"

// Функция чтения одного UTF-8 символа или ESC последовательности в buf
int read_utf8_char_or_esc_seq(int fd, char* buf, int buf_size);

#endif
