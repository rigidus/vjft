// iface.c

#include "iface.h"
#include "msg.h"

/**
   Подсчитывает сколько строк и столбцов необходимо для
   вывода текста в текстовое окно максималальной ширины
   max_width и в какой из этих строк будет расположен курсор
*/
void calc_display_size(const char* text, const int max_width,
                       const int cursor_pos,
                       int* need_cols, int* need_rows,
                       int* cursor_row, int* cursor_col) {
    const char* byte_pos = text; // Указатель на байтовую позицию
    int cur_row = 1; // Текущая строка, она же счетчик строк
    int cur_col = 0; // Длина текущей строки
    int max_col = 0; // Максимальная найденная длина строки

    for (int cur_pos = 0; cur_pos <= utf8_strlen(text); cur_pos++)
    {
        size_t char_len = utf8_char_length(byte_pos);
        char32_t codepoint = 0;
        utf8_decode(byte_pos, char_len, &codepoint);

        if (cur_pos == cursor_pos) {
            // Дошли до позиции курсора?
            *cursor_row = cur_row; // Вернуть текущую строку
            *cursor_col = cur_col; // Вернуть текущий столбец
        }

        cur_col++; // Увеличение позиции в текущей строке
        if (cur_col > max_col) {
            // Если эта строка длиннее чем ранее встреченные
            // Обновить максимальную длину строки
            max_col = cur_col;
        }

        // Символ переноса строки или правая граница?
        if ((codepoint == '\n') || (cur_col >= max_width)) {
            cur_col = 0;  // Сброс длины строки для новой строки
            cur_row++;    // Увеличение счетчика строк
        }

        // Терминатор строки?
        if (codepoint == '\0') {
            cur_col++;
            break;
        }

        // Переходим к следующему символу
        byte_pos += char_len; // Инкремент указателя
    }

    // Если последняя строка оказалась самой длинной
    if (cur_col > max_col) {
        max_col = cur_col;
    }

    *need_rows = cur_row;
    *need_cols = max_col;
}


void set_highlight_color() {
    // Инвертирование цветов (меняет фон и текст местами)
    printf("\033[7m");
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
                     int marker_pos)
{
    const char* byte_pos = text; // Указатель на байтовую позицию
    int  rel_row   = 0; // Текущая строка, она же счётчик строк
    int  rel_col   = 0; // Текущий столбец
    int  sel_start = min(cursor_pos, marker_pos);
    int  sel_end   = max(cursor_pos, marker_pos);
    bool is_highlighted = false; // Флаг выделения
    int  scr_row   = 0; // abs_x
    int  scr_col   = 0; // abs_y + rel_row - from_row - 1
    int  fg_color = DEFAULT_FG_COLOR;
    int  bg_color = DEFAULT_BG_COLOR;

    bool is_not_skipped_row() {
        return (rel_row > from_row);
    }

    void fullfiller () {
        if (is_not_skipped_row()) {
            // Если мы не пропускаем
            // Пока не правая граница - заполняем
            while (rel_col <= rel_max_width) {
                /* moveCursor(scr_col, scr_row); */
                buffered_putchar(back_buffer, scr_row, scr_col,
                                 FILLER, fg_color, bg_color);
                rel_col++; // Увеличиваем счётчик длины строки
            }
        }
    }

    void inc_rel_row() {
        rel_col = 0; // Сброс длины строки для новой строки
        rel_row++;   // Увеличение счётчика строк
        if (is_not_skipped_row()) { // Если мы не пропускаем
            /* moveCursor(abs_x, abs_y + rel_row - from_row - 1); */
            scr_col = abs_x;
            scr_row = abs_y + rel_row - from_row;
        }
    }

    inc_rel_row(); // Курсор на начальную позицию

    for (int cur_pos = 0; cur_pos <= utf8_strlen(text); cur_pos++)
    {
        size_t char_len = utf8_char_length(byte_pos);
        char32_t codepoint = 0;
        utf8_decode(byte_pos, char_len, &codepoint);

        if (scr_row    >= 0  &&  scr_row < back_buffer->rows
            && scr_col >= 0  &&  scr_col < back_buffer->cols) {

            int fg_color = DEFAULT_FG_COLOR;
            int bg_color = DEFAULT_BG_COLOR;

            if (marker_pos != -1 // selected text
                && cur_pos >= sel_start && cur_pos < sel_end)
            {
                bg_color = HIGHLIGHT_BG_COLOR;
                fg_color = HIGHLIGHT_FG_COLOR;
            }

            if (cur_pos == cursor_pos) { // is_cursor
                fg_color = BRIGHT_WHITE_FG_COLOR;
                bg_color = GREEN_BG_COLOR;
            }

            // Если текущая строка достигает максимальной
            // ширины отображения
            if (rel_col >= rel_max_width) {
                inc_rel_row();
            }

            // Если максимальное количество строк достигнуто
            if (rel_row > (rel_max_rows + from_row)) {
                break;  // Прекращаем вывод
            }

            if (codepoint == '\n') {
                // Если текущий символ - перевод строки
                fullfiller();  // Заполняем оставшееся филлером
                inc_rel_row(); // Переходим на следующую строку
            } else {
                if (codepoint == '\0') {
                    // Если текущий символ - завершающий нулевой байт
                    if (is_not_skipped_row()) {
                        // Если мы не пропускаем
                        // Выводим символ END_OF_TEXT
                        /* moveCursor(scr_col, scr_row); */
                        buffered_putchar(back_buffer, scr_row,
                                         scr_col, U'⍿',
                                         fg_color, bg_color);
                        scr_col++;
                    }
                    if (is_highlighted) {
                        // Убираем выделение если оно есть
                        reset_highlight_color();
                    }
                    // выходим из цикла
                    break;
                } else {
                    // Обычный печатаемый символ
                    if (is_not_skipped_row()) {
                        // Если мы не пропускаем
                        // Выводим UTF8-символ
                        /* moveCursor(scr_col, scr_row); */
                        buffered_putchar(back_buffer, scr_row,
                                         scr_col, codepoint,
                                         fg_color, bg_color);
                        scr_col++;
                    }
                }
                rel_col++; // Увеличиваем счётчик длины строки
            }
            byte_pos += char_len; // Переходим к след символу
        }
    }

    // Заполнение оставшейся части строки до конца,
    // если необходимо
    if (rel_col != 0) {
        fullfiller();
    }
}


int display_message(MsgNode* msgnode, int x, int y,
                    int max_width, int max_height)
{
    int needed_cols, needed_rows, cursor_row, cursor_col;
    calc_display_size(msgnode->text, max_width, 0,
                      &needed_cols, &needed_rows,
                      &cursor_row, &cursor_col);

    int display_start_row =
        (needed_rows > max_height) ?
        needed_rows - max_height : 0;
    int actual_rows = min(needed_rows, max_height);

    display_wrapped(msgnode->text, x, y, max_width,
                    actual_rows, display_start_row,
                    msgnode->cursor_pos, msgnode->marker_pos);

    return actual_rows;
}

int display_message_with_frame(MsgNode* msgnode, int x, int y,
                               int max_width, int max_height)
{
    // Доступная ширина для текста внутри рамки
    int rel_max_width = max_width - 4;
    if (rel_max_width <= 0) {
        // Недостаточно места для отображения текста
        return 0;
    }

    // Вычисляем размеры текста с учётом доступной ширины
    int needed_cols, needed_rows, cursor_row, cursor_col;
    calc_display_size(msgnode->text, rel_max_width,
                      msgnode->cursor_pos,
                      &needed_cols, &needed_rows,
                      &cursor_row, &cursor_col);

    // Верхний левый угол
    buffered_putchar(back_buffer,
                     y,
                     x,
                     U'┌', DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
    // Верхний правый угол
    buffered_putchar(back_buffer,
                     y,
                     x + max_width-1,
                     U'┐', DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);

    /* char fc_text[1024] = {0}; */
    /* snprintf(fc_text, 1024, */
    /*          "needed_rows=%d", */
    /*          needed_rows); */
    /* pushMessage(&msgList, strdup(fc_text)); */

    // Нижний левый угол
    buffered_putchar(back_buffer,
                     y + needed_rows + 2 - 1,
                     x,
                     U'└', DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
    // Нижний правый угол
    buffered_putchar(back_buffer,
                     y + needed_rows + 2 - 1,
                     x + max_width-1,
                     U'┘', DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);

    // Верхняя и нижняя горизонтальные линии
    drawHorizontalLine(x + 1,
                       y,
                       max_width - 2, U'─');
    drawHorizontalLine(x + 1,
                       y + needed_rows + 2 - 1,
                       max_width - 2, U'─');

    // Левая и правая вертикальные линии
    drawVerticalLine(x,
                     y+1,
                     needed_rows + 2 - 2,
                     U'│');
    drawVerticalLine(x + max_width -1,
                     y+1,
                     needed_rows + 2 - 2,
                     U'│');

    // Отображаем текст внутри рамки с учётом отступов
    display_wrapped(msgnode->text, x + 2, y,
                    rel_max_width, needed_rows,
                    0,  // from_row
                    -1, //msgnode->cursor_pos,
                    -1  //msgnode->marker_pos
        );

    return max_height; // Возвращаем высоту с рамкой
}


void drawHorizontalLine(int x, int y, int cols, char32_t sym) {
    for (int i = x; i < x + cols; i++) {
        buffered_putchar(back_buffer, y, i, sym,
                         DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
    }
}

void drawVerticalLine(int x, int y, int rows, char32_t sym) {
    for (int i = y; i < y + rows; i++) {
        buffered_putchar(back_buffer, i, x, sym,
                         DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
    }
}


ScreenBuffer *front_buffer = NULL;
ScreenBuffer *back_buffer = NULL;


ScreenBuffer* create_screen_buffer(int rows, int cols) {
    ScreenBuffer *buffer = malloc(sizeof(ScreenBuffer));
    if (!buffer) return NULL;

    buffer->rows = rows;
    buffer->cols = cols;

    buffer->cells = malloc(rows * sizeof(Cell*));
    if (!buffer->cells) {
        free(buffer);
        return NULL;
    }

    for (int i = 0; i < rows; i++) {
        buffer->cells[i] = malloc(cols * sizeof(Cell));
        if (!buffer->cells[i]) {
            // Освобождаем ранее выделенную память
            for (int j = 0; j < i; j++) {
                free(buffer->cells[j]);
            }
            free(buffer->cells);
            free(buffer);
            return NULL;
        }
        // Инициализируем ячейки
        for (int j = 0; j < cols; j++) {
            buffer->cells[i][j].sym = DEFAULT_SYMBOL;
            buffer->cells[i][j].fg_color = DEFAULT_FG_COLOR;
            buffer->cells[i][j].bg_color = DEFAULT_BG_COLOR;
            buffer->cells[i][j].needs_update = true;
        }
    }

    return buffer;
}


void free_screen_buffer(ScreenBuffer *buffer) {
    if (!buffer) return;

    for (int i = 0; i < buffer->rows; i++) {
        free(buffer->cells[i]);
    }
    free(buffer->cells);
    free(buffer);
}


void clear_screen_buffer(ScreenBuffer *buffer) {
    for (int i = 0; i < buffer->rows; i++) {
        for (int j = 0; j < buffer->cols; j++) {
            buffer->cells[i][j].sym = DEFAULT_SYMBOL;
            buffer->cells[i][j].fg_color = DEFAULT_FG_COLOR;
            buffer->cells[i][j].bg_color = DEFAULT_BG_COLOR;
            buffer->cells[i][j].needs_update = true;
        }
    }
}

void init_buffers(int rows, int cols) {
    front_buffer = create_screen_buffer(rows, cols);
    back_buffer = create_screen_buffer(rows, cols);
    if (!front_buffer || !back_buffer) {
        exit(EXIT_FAILURE);
    }
}


void resize_buffers(int new_rows, int new_cols) {
    free_screen_buffer(front_buffer);
    free_screen_buffer(back_buffer);
    init_buffers(new_rows, new_cols);
}


void buffered_putchar(ScreenBuffer *buffer, int row, int col,
                      char32_t sym, int fg_color, int bg_color) {
    if (row >= 0     && row < buffer->rows
        && col >= 0  && col < buffer->cols)
    {
        Cell *cell = &buffer->cells[row][col];
        if (cell->sym != sym
            || cell->fg_color != fg_color
            || cell->bg_color != bg_color)
        {
            cell->sym = sym;
            cell->fg_color = fg_color;
            cell->bg_color = bg_color;
            cell->needs_update = true;
        }
    }
}


// Функция для преобразования char32_t в UTF-8
size_t char32_to_utf8(char32_t c, char *out) {
    if (c <= 0x7F) {
        out[0] = (char)c;
        return 1;
    } else if (c <= 0x7FF) {
        out[0] = (char)(0xC0 | ((c >> 6) & 0x1F));
        out[1] = (char)(0x80 | (c & 0x3F));
        return 2;
    } else if (c <= 0xFFFF) {
        out[0] = (char)(0xE0 | ((c >> 12) & 0x0F));
        out[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[2] = (char)(0x80 | (c & 0x3F));
        return 3;
    } else if (c <= 0x10FFFF) {
        out[0] = (char)(0xF0 | ((c >> 18) & 0x07));
        out[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[3] = (char)(0x80 | (c & 0x3F));
        return 4;
    }
    // Неверная кодовая точка Unicode
    return 0;
}


void render_screen() {
    // Скрываем курсор, чтобы не было мерцания
    printf("\033[?25l");
    // Перемещаем курсор в верхний левый угол
    printf("\033[H");

    for (int i = 0; i < back_buffer->rows; i++) {
        for (int j = 0; j < back_buffer->cols; j++) {
            Cell *back_cell = &back_buffer->cells[i][j];
            Cell *front_cell = &front_buffer->cells[i][j];

            if (true
                || back_cell->needs_update
                || back_cell->sym != front_cell->sym
                || back_cell->fg_color != front_cell->fg_color
                || back_cell->bg_color != front_cell->bg_color) {

                // Перемещаем курсор к позиции
                printf("\033[%d;%dH", i + 1, j + 1);

                // Устанавливаем цвета
                printf("\033[%dm", back_cell->fg_color);
                printf("\033[%dm", back_cell->bg_color);

                // Выводим символ
                char utf8_bytes[4];
                size_t utf8_length =
                    char32_to_utf8(back_cell->sym, utf8_bytes);

                if (utf8_length > 0) {
                    // Используем write для вывода на stdout
                    ssize_t written =
                        write(STDOUT_FILENO, utf8_bytes, utf8_length);
                    if (written == -1) {
                        perror("Error : stdout write");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    /* fprintf(stderr, "Error codepoint"); */
                    /* exit(EXIT_FAILURE); */
                }

                // Обновляем front_buffer
                *front_cell = *back_cell;
                back_cell->needs_update = false;
            }
        }
    }

    // Показываем курсор
    printf("\033[?25h");
    fflush(stdout);

    // Теперь можно менять местами буферы, если нужно
    // Но в нашем случае мы обновили front_buffer,
    // поэтому можем не менять
}
