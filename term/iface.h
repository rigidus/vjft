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

#define FILLER U':' // '▒'

void render_text_window(const char* text,
                        int window_x, int window_y,
                        int window_width, int window_height,
                        int cursor_pos, int marker_pos);

// Основная функция для вывода текста с учётом переноса строк
void display_wrapped(const char* text, int abs_x, int abs_y,
                     int rel_max_width, int rel_max_rows,
                     int from_row, int cursor_pos,
                     int marker_pos);

int display_message(MsgNode* message, int x, int y,
                    int max_width, int max_height);


#define DEFAULT_SYMBOL U' ' // '░'
#define DEFAULT_FG_COLOR 32
#define DEFAULT_BG_COLOR 40
#define HIGHLIGHT_FG_COLOR 92
#define HIGHLIGHT_BG_COLOR 100

typedef struct {
    char32_t sym;
    int fg_color;      // Цвет текста (если нужно)
    int bg_color;      // Цвет фона (если нужно)
    bool needs_update; // Флаг, что эта ячейка изменилась
} Cell;

typedef struct {
    int rows;
    int cols;
    Cell **cells; // Двумерный массив ячеек
} ScreenBuffer;

extern ScreenBuffer *front_buffer;
extern ScreenBuffer *back_buffer;

ScreenBuffer* create_screen_buffer(int rows, int cols);
void free_screen_buffer(ScreenBuffer *buffer);
void clear_screen_buffer(ScreenBuffer *buffer);
void init_buffers(int rows, int cols);
void resize_buffers(int new_rows, int new_cols);

void buffered_putchar(ScreenBuffer *buffer, int row, int col,
                      char32_t sym, int fg_color, int bg_color);

size_t char32_to_utf8(char32_t c, char *out);
void render_screen();

#endif
