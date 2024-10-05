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

extern MsgList msgList;

typedef struct {
    int byte_offset_start;
    int text_pos_start;
    int byte_offset_end;
    int text_pos_end;
} LineInfo;


#define FILLER U':' // '▒'

int get_lines (const char* text, int max_col, int max_row,
               LineInfo* lines);

void render_text_window(const char* text,
                        int window_x, int window_y,
                        int max_col, int max_row,
                        int cursor_pos, int marker_pos,
                        int* scroll_offset);

// Основная функция для вывода текста с учётом переноса строк
void display_wrapped(const char* text, int abs_x, int abs_y,
                     int rel_max_width, int rel_max_rows,
                     int from_row, int cursor_pos,
                     int marker_pos);

int display_message(MsgNode* message, int x, int y,
                    int max_width, int max_height);


// А чтобы раскрасить буквы, используйте формат
// \033[число;число.
// Обычно вам нужно написать только одну цифру.
// Цвет фона и цвет шрифта определяются в зависимости
// от значения, также вы можете установить сразу цвет
// фона и шрифта.

/* 0: Сбросить все цвета и стили. */
/* 1: жирный/яркий */
/* 3: курсив */
/* 4: подчеркнуть */
/* 7: Инверсия (инвертировать цвет шрифта/цвет фона) */
/* 9: Рисование горизонтальной линии */
/* 22: Удалить жирный шрифт */
/* 23: Удалить курсив */
/* 24: Удалить подчеркивание */
/* 27: Удалить инверсию */
/* 29: Удалить горизонтальные линии */
/* 30: Цвет шрифта: черный */
/* 31: Цвет шрифта: красный */
/* 32: Цвет шрифта: Зеленый */
/* 33: Цвет шрифта: Желтый. */
/* 34: Цвет шрифта: Синий. */
/* 35: Цвет шрифта: пурпурный (розовый). */
/* 36: Цвет шрифта: голубой (голубой) */
/* 37: Цвет шрифта: Белый */
/* 39: Установить цвет шрифта по умолчанию. */
/* 40: Цвет фона: черный */
/* 41: Цвет фона: красный. */
/* 42: Цвет фона: Зеленый. */
/* 43: Цвет фона: желтый. */
/* 44: Цвет фона: синий. */
/* 45: Цвет фона: Розовый. */
/* 46: Цвет фона: голубой. */
/* 47: Цвет фона: Белый. */
/* 49: Установить цвет фона по умолчанию.  */

#define DEFAULT_SYMBOL U' ' // '░'
#define DEFAULT_FG_COLOR 32
#define DEFAULT_BG_COLOR 40
#define HIGHLIGHT_FG_COLOR 92
#define HIGHLIGHT_BG_COLOR 100

#define BLACK_BG_COLOR           40
#define RED_BG_COLOR             41
#define GREEN_BG_COLOR           42
#define YELLOW_BG_COLOR          43
#define BLUE_BG_COLOR            44
#define MAGENTA_BG_COLOR         45
#define CYAN_BG_COLOR            46
#define WHITE_BG_COLOR           47
#define BRIGHT_BLACK_BG_COLOR    100
#define BRIGHT_RED_BG_COLOR      101
#define BRIGHT_GREEN_BG_COLOR    102
#define BRIGHT_YELLOW_BG_COLOR   103
#define BRIGHT_BLUE_BG_COLOR     104
#define BRIGHT_MAGENTA_BG_COLOR  105
#define BRIGHT_CYAN_BG_COLOR     106
#define BRIGHT_WHITE_BG_COLOR    107

#define BLACK_FG_COLOR           30
#define RED_FG_COLOR             31
#define GREEN_FG_COLOR           32
#define YELLOW_FG_COLOR          33
#define BLUE_FG_COLOR            34
#define MAGENTA_FG_COLOR         35
#define CYAN_FG_COLOR            36
#define WHITE_FG_COLOR           37
#define BRIGHT_BLACK_FG_COLOR    90
#define BRIGHT_RED_FG_COLOR      91
#define BRIGHT_GREEN_FG_COLOR    92
#define BRIGHT_YELLOW_FG_COLOR   93
#define BRIGHT_BLUE_FG_COLOR     94
#define BRIGHT_MAGENTA_FG_COLOR  95
#define BRIGHT_CYAN_FG_COLOR     96
#define BRIGHT_WHITE_FG_COLOR    97


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
