// iface.c

#include "iface.h"

/**
   Подсчитывает сколько строк и столбцов необходимо для вывода текста
   в текстовое окно максималальной ширины max_width и в какой из этих
   строк будет расположен курсор
*/
void calc_display_size(const char* text, int max_width, int cursor_pos,
                       int* need_cols, int* need_rows,
                       int* cursor_row, int* cursor_col)
{
    int cur_text_pos = -1; // Текущий индекс выводимого символа строки
    int cur_row = 1; // Текущая строка, она же счетчик строк
    int cur_col = 0; // Длина текущей строки
    int max_col = 0; // Максимальная найденная длина строки

    for (const char* p = text; ; ) {
        size_t char_len = utf8_char_length(p);

        cur_text_pos++; // Увеличение текущей позиции в тексте
        if (cur_text_pos == cursor_pos) { // Дошли до позиции курсора?
            *cursor_row = cur_row;        // Вернуть текущую строку
            *cursor_col = cur_col;        // Вернуть текущий столбец
        }

        cur_col++; // Увеличение позиции в текущей строке
        // Если эта строка длиннее чем все ранее встреченные
        if (cur_col > max_col) {
            max_col = cur_col; // Обновить максимальную длину строки
        }

        // Символ переноса строки или правая граница?
        if ((*p == '\n') || (cur_col >= max_width)) {
            cur_col = 0;  // Сброс длины строки для новой строки
            cur_row++;    // Увеличение счетчика строк
        }

        // Терминатор строки?
        if (*p == 0) {
            cur_col++;
            break;
        }

        p += char_len; // Переходим к следующему символу
    }

    // Если последняя строка оказалась самой длинной
    if (cur_col > max_col) {
        max_col = cur_col;
    }

    *need_rows = cur_row;
    *need_cols = max_col;
}



void set_highlight_color() {
    printf("\033[7m"); // Инвертирование цветов (меняет фон и текст местами)
}

void reset_highlight_color() {
    printf("\033[0m"); // Сброс всех атрибутов к стандартным
}

/**
   Вывод текста в текстовое окно

   abs_x, abs_y - левый верхний угол окна
   rel_max_width, rel_max_rows - размер окна
   from_row - строка внутри text с которой начинается вывод
   (он окончится когда закончится текст или окно, а окно
   закончится, когда будет выведено max_width строк)
*/

// Основная функция для вывода текста с учётом переноса строк
void display_wrapped(const char* text, int abs_x, int abs_y,
                     int rel_max_width, int rel_max_rows,
                     int from_row, int cursor_pos,
                     int shadow_cursor_pos)
{
    int cur_pos   = 0; // Текущий индекс символа в строке
    int rel_row   = 0;  // Текущая строка, она же счётчик строк
    int rel_col   = 0;  // Текущий столбец
    int sel_start = min(cursor_pos, shadow_cursor_pos);
    int sel_end   = max(cursor_pos, shadow_cursor_pos);
    bool is_highlighted = false; // Флаг выделения

    bool is_not_skipped_row() {
        return (rel_row > from_row);
    }

    void fullfiller () {
        if (is_not_skipped_row()) { // Если мы не пропускаем
            while (rel_col < rel_max_width) { // Пока не правая граница
                putchar(FILLER); // Заполняем
                rel_col++; // Увеличиваем счётчик длины строки
            }
        }
    }

    void inc_rel_row() {
        rel_col = 0; // Сброс длины строки для новой строки
        rel_row++;   // Увеличение счётчика строк
        if (is_not_skipped_row()) { // Если мы не пропускаем
            moveCursor(abs_x, abs_y + rel_row - from_row - 1);
        }
    }

    inc_rel_row(); // Курсор на начальную позицию


    for (const char* p = text; ; ) {
        size_t char_len = utf8_char_length(p);

        // Проверяем, нужно ли изменить цвет фона для этого символа
        if (cur_pos >= sel_start && cur_pos < sel_end) {
            if (!is_highlighted) {
                set_highlight_color();
                is_highlighted = true;
            }
        } else {
            if (is_highlighted) {
                reset_highlight_color();
                is_highlighted = false;
            }
        }

        // Если текущая строка достигает максимальной ширины отображения
        if (rel_col >= rel_max_width) {
            inc_rel_row();
        }

        // Если максимальное количество строк достигнуто
        if (rel_row > (rel_max_rows + from_row)) {
            break;  // Прекращаем вывод
        }

        if (*p == '\n') {  // Если текущий символ - перевод строки
            fullfiller();  // Заполняем оставшееся филлером
            inc_rel_row(); // Переходим на следующую строку
            p += char_len; // Переходим к следующему символу
            cur_pos++;
        } else {
            if (*p == '\0') {  // Если текущий символ - завершающий нулевой байт
                if (is_not_skipped_row()) { // Если мы не пропускаем
                    fputs("⍿", stdout); // Выводим символ END_OF_TEXT
                }
                if (is_highlighted) { // Убираем выделение если оно есть
                    reset_highlight_color();
                }
                // выходим из цикла
                break;
            } else {
                // Обычный печатаемый символ
                if (is_not_skipped_row()) { // Если мы не пропускаем
                    fwrite(p, 1, char_len, stdout); // Выводим символ (UTF-8)
                }
            }
            rel_col++; // Увеличиваем счётчик длины строки
            p += char_len; // Переходим к следующему символу
            cur_pos++;
        }
    }


    // Заполнение оставшейся части строки до конца, если необходимо
    if (rel_col != 0) {
        fullfiller();
    }
}


int display_message(MsgNode* message, int x, int y, int max_width, int max_height) {
    if (message == NULL || message->message == NULL) return 0;

    int needed_cols, needed_rows, cursor_row, cursor_col;
    calc_display_size(message->message, max_width, 0, &needed_cols, &needed_rows, &cursor_row, &cursor_col);

    int display_start_row = (needed_rows > max_height) ? needed_rows - max_height : 0;
    int actual_rows = min(needed_rows, max_height);

    display_wrapped(message->message, x, y, max_width, actual_rows, display_start_row,
                    message->cursor_pos, message->shadow_cursor_pos);

    return actual_rows;
}

void drawHorizontalLine(int cols, int y, char sym) {
    moveCursor(1, y);
    for (int i = 0; i < cols; i++) {
        putchar(sym);
        fflush(stdout);
    }
}
