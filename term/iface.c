// iface.c

#include "iface.h"

/**
   Подсчитывает сколько строк и столбцов необходимо для
   вывода текста в текстовое окно максималальной ширины
   max_width и в какой из этих строк будет расположен курсор
*/
void calc_display_size(const char* text, int max_width,
                       int cursor_pos,
                       int* need_cols, int* need_rows,
                       int* cursor_row, int* cursor_col) {
    int cur_text_pos = -1; // Текущий индекс выводимого символа
    int cur_row = 1; // Текущая строка, она же счетчик строк
    int cur_col = 0; // Длина текущей строки
    int max_col = 0; // Максимальная найденная длина строки

    for (const char* p = text; ; ) {
        size_t char_len = utf8_char_length(p);

        cur_text_pos++; // Увеличение текущей позиции в тексте
        if (cur_text_pos == cursor_pos) {
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
    // Инвертирование цветов (меняет фон и текст местами)
    printf("\033[7m");
}

void reset_highlight_color() {
    printf("\033[0m"); // Сброс всех атрибутов к стандартным
}


void render_text_window(const char* text,
                        int window_x, int window_y,
                        int window_width, int window_height,
                        int cursor_pos, int marker_pos,
                        int* scroll_offset) {
    int text_pos = 0;     // Position in the text (in characters)
    int byte_offset = 0;  // Byte offset in the text
    int current_col = 0;
    int max_row = window_height;
    int max_col = window_width;
    int text_length = utf8_strlen(text);
    int sel_start = min(cursor_pos, marker_pos);
    int sel_end = max(cursor_pos, marker_pos);
    bool is_highlighted = false;

    // Calculate the total number of lines and positions of each line start
    // so we can properly handle scrolling
    typedef struct {
        int byte_offset_start;
        int text_pos_start;
        int byte_offset_end;
        int text_pos_end;
    } LineInfo;

    // Estimate maximum number of lines
    int max_lines = text_length + 1;
    LineInfo* lines = malloc(max_lines * sizeof(LineInfo));
    int lindex = 0;

    int start_byte_offset = 0;
    int start_text_pos = 0;

    // First pass: collect line start positions
    while (byte_offset < strlen(text)) {
        // length of next character
        int char_len = utf8_char_length(text + byte_offset);
        char32_t codepoint = 0;
        utf8_decode(text + byte_offset, char_len, &codepoint);

        // Check if we need to wrap the line
        if (current_col >= max_col || codepoint == '\n') {
            lines[lindex].byte_offset_start = start_byte_offset;
            lines[lindex].text_pos_start = start_text_pos;
            lines[lindex].byte_offset_end = byte_offset;
            lines[lindex].text_pos_end = text_pos;
            lindex++;
            current_col = 0;
            if (codepoint == '\n') {
                byte_offset += char_len;
                text_pos++;
            }
            start_byte_offset = byte_offset;
            start_text_pos = text_pos;
        } else {
            current_col++;
            byte_offset += char_len;
            text_pos++;
        }
    }
    lines[lindex].byte_offset_start = start_byte_offset;
    lines[lindex].text_pos_start = start_text_pos;
    lines[lindex].byte_offset_end = byte_offset;
    lines[lindex].text_pos_end = text_pos;
    lindex++;

    /* // dbg  out */
    /* for (int current_row = 0; current_row < lindex; current_row++) { */
    /*     char is_text[1024] = {0}; */
    /*     char sub[1024] = {0}; */
    /*     memcpy(&sub, text + lines[current_row].byte_offset_start, */
    /*            lines[current_row].byte_offset_end */
    /*            - lines[current_row].byte_offset_start); */
    /*     snprintf( */
    /*         is_text, 1024, */
    /*         "[%d] %d..%d '%s' (%d)", */
    /*         current_row, */
    /*         lines[current_row].byte_offset_start, */
    /*         lines[current_row].byte_offset_end, */
    /*         &sub, */
    /*         lines[current_row].byte_offset_end */
    /*         - lines[current_row].byte_offset_start */
    /*         ); */
    /*     pushMessage(&msgList, is_text); */
    /* } */
    /* // end dbg out */


    // Adjust scroll_offset to ensure cursor is visible
    if (cursor_pos != -1) {
        int cursor_line = 0;
        for (int i = 0; i < lindex; i++) {
            if (cursor_pos >= lines[i].text_pos_start
                && (i == lindex - 1
                    || cursor_pos < lines[i].text_pos_end))
            {
                cursor_line = i;
                break;
            }
        }
        if (cursor_line < *scroll_offset) {
            *scroll_offset = cursor_line;
        } else if (cursor_line >= *scroll_offset + max_row) {
            *scroll_offset = cursor_line - max_row + 1;
        }
    }

    // Second pass: render the visible lines
    for (int current_row = 0; current_row < max_row; current_row++) {
        int line_idx = *scroll_offset + current_row;

        // Render the line
        byte_offset = lines[line_idx].byte_offset_start;
        text_pos = lines[line_idx].text_pos_start;
        current_col = 0;

        while (current_col < max_col
               && text_pos < text_length
               && current_row < lindex)
        {
            int char_len = utf8_char_length(text + byte_offset);
            char32_t codepoint = 0;
            utf8_decode(text + byte_offset, char_len, &codepoint);

            if (codepoint == '\n') {
                byte_offset += char_len;
                text_pos++;
                break; // Move to next line
            }

            // Check for selection highlighting
            if (marker_pos >= 0) {
                if (text_pos >= sel_start && text_pos < sel_end) {
                    is_highlighted = true;
                } else {
                    is_highlighted = false;
                }
            }

            // Check if this is the cursor position
            bool is_cursor = (text_pos == cursor_pos);

            // Set the colors based on cursor and highlighting
            int fg_color = DEFAULT_FG_COLOR;
            int bg_color = DEFAULT_BG_COLOR;
            if (is_cursor) {
                /* fg_color = DEFAULT_BG_COLOR; */
                /* bg_color = DEFAULT_FG_COLOR; */
                fg_color = BRIGHT_WHITE_FG_COLOR;
                bg_color = MAGENTA_BG_COLOR;
            } else if (is_highlighted) {
                fg_color = HIGHLIGHT_FG_COLOR;
                bg_color = HIGHLIGHT_BG_COLOR;
            }

            // Write the character to the back_buffer
            buffered_putchar(back_buffer,
                             window_y + current_row,
                             window_x + current_col,
                             codepoint,
                             fg_color,
                             bg_color);

            // Advance positions
            current_col++;
            text_pos++;
            byte_offset += char_len;
        }

        // Fill the rest of the line with spaces
        for (; current_col < max_col; current_col++) {
            buffered_putchar(back_buffer,
                             window_y + current_row,
                             window_x + current_col,
                             '_',
                             DEFAULT_FG_COLOR,
                             DEFAULT_BG_COLOR);
        }
    }

    free(lines);
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
    int  cur_pos   = 0; // Текущий индекс символа в строке
    int  rel_row   = 0; // Текущая строка, она же счётчик строк
    int  rel_col   = 0; // Текущий столбец
    int  sel_start = min(cursor_pos, marker_pos);
    int  sel_end   = max(cursor_pos, marker_pos);
    bool is_highlighted = false; // Флаг выделения
    int  scr_row   = 0;
    int  scr_col   = 0;
    int  fg_color = DEFAULT_FG_COLOR;
    int  bg_color = DEFAULT_BG_COLOR;


    bool is_not_skipped_row() {
        return (rel_row > from_row);
    }

    void fullfiller () {
        if (is_not_skipped_row()) {
            // Если мы не пропускаем
            // Пока не правая граница
            while (rel_col < rel_max_width) {
                moveCursor(scr_col, scr_row);
                /* putchar(FILLER); // Заполняем */
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
            scr_row = abs_y + rel_row - from_row - 1;
        }
    }

    inc_rel_row(); // Курсор на начальную позицию

    for (const char* p = text; ; ) {
        size_t char_len = utf8_char_length(p);

        /* if (scr_row >= 0 */
        /*     && scr_row < back_buffer->rows */
        /*     && scr_col >= 0 */
        /*     && scr_col < back_buffer->cols) */
        /* { */
            /* // Учитываем выделение */
            /* int fg_color = DEFAULT_FG_COLOR; */
            /* int bg_color = DEFAULT_BG_COLOR; */
            /* if (marker_pos != -1 */
            /*     && cur_pos >= sel_start */
            /*     && cur_pos < sel_end) */
            /* { */
            /*     bg_color = HIGHLIGHT_BG_COLOR; */
            /*     fg_color = HIGHLIGHT_FG_COLOR; */
            /* } */

            /* // Записываем символ в back_buffer */
            /* for (size_t i = 0; i < char_len; i++) { */
            /*     buffered_putchar(back_buffer, scr_row, */
            /*                      scr_col, p[i], fg_color, */
            /*                      bg_color); */
            /*     scr_col++; */
            /*     if (scr_col >= back_buffer->cols) { */
            /*         break; */
            /*     } */
            /* } */
        /* } */

        // Проверяем, нужно ли изменить цвет фона
        // для этого символа
        if (marker_pos != -1) {
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

        if (*p == '\n') {
            // Если текущий символ - перевод строки
            fullfiller();  // Заполняем оставшееся филлером
            inc_rel_row(); // Переходим на следующую строку
            p += char_len; // Переходим к следующему символу
            cur_pos++;
        } else {
            if (*p == '\0') {
                // Если текущий символ - завершающий нулевой байт
                if (is_not_skipped_row()) {
                    // Если мы не пропускаем
                    // Выводим символ END_OF_TEXT
                    /* moveCursor(scr_col, scr_row); */
                    /* fputs("⍿", stdout); */
                    buffered_putchar(back_buffer, scr_row,
                                     scr_col, U'⌡',
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
                    /* fwrite(p, 1, char_len, stdout); */
                    buffered_putchar(back_buffer, scr_row,
                                     scr_col, p[0],
                                     fg_color, bg_color);
                    scr_col++;
                }
            }
            rel_col++; // Увеличиваем счётчик длины строки
            p += char_len; // Переходим к следующему символу
            cur_pos++;
        }
    }

    // Заполнение оставшейся части строки до конца,
    // если необходимо
    if (rel_col != 0) {
        fullfiller();
    }
}


int display_message(MsgNode* msgnode, int x, int y,
                    int max_width, int max_height) {
    if (msgnode == NULL || msgnode->text == NULL) {
        return 0;
    }

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

void drawHorizontalLine(int cols, int y, char sym) {
    /* moveCursor(1, y); */
    /* for (int i = 0; i < cols; i++) { */
    /*     /\* putchar(sym); *\/ */
    /*     buffered_putchar(back_buffer, y - 1, i, sym, DEFAULT_FG_COLOR, DEFAULT_BG_COLOR); */
    /*     fflush(stdout); */
    /* } */
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
