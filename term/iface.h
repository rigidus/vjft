// iface.h

#ifndef IFACE_H
#define IFACE_H

#include "all_libs.h"
#include "utf8.h"
#include "term.h"
#include "msg.h"


/**
   Подсчитывает сколько строк и столбцов необходимо для вывода текста
   в текстовое окно максималальной ширины max_width и в какой из этих
   строк будет расположен курсор
*/
void calc_display_size(const char* text, int max_width, int cursor_pos,
                       int* need_cols, int* need_rows,
                       int* cursor_row, int* cursor_col);

/**
   Вывод текста в текстовое окно

   abs_x, abs_y - левый верхний угол окна
   rel_max_width, rel_max_rows - размер окна
   from_row - строка внутри text с которой начинается вывод
   (он окончится когда закончится текст или окно, а окно
   закончится, когда будет выведено max_width строк)
*/

#define FILLER ':'

// Основная функция для вывода текста с учётом переноса строк
void display_wrapped(const char* text, int abs_x, int abs_y,
                     int rel_max_width, int rel_max_rows,
                     int from_row, int cursor_pos,
                     int shadow_cursor_pos);

int display_message(MessageNode* message, int x, int y, int max_width, int max_height);

#endif
