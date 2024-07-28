typedef struct {
    char* buffer;     // Указатель на массив символов
    size_t capacity;  // Общая вместимость буфера
    size_t gap_start; // Начало разрыва
    size_t gap_end;   // Конец разрыва
} GapBuffer;

void gap_buffer_init(GapBuffer* gb, size_t initial_capacity) {
    gb->buffer = (char*)malloc(initial_capacity * sizeof(char));
    memset(gb->buffer, 0, initial_capacity * sizeof(char));
    gb->capacity = initial_capacity;
    gb->gap_start = 0;
    gb->gap_end = initial_capacity - 1;
}

void gap_buffer_free(GapBuffer* gb) {
    free(gb->buffer);
    gb->buffer = NULL;
    gb->capacity = 0;
    gb->gap_start = 0;
    gb->gap_end = 0;
}

void gap_buffer_insert(GapBuffer* gb, char c) {
    if (gb->gap_start == gb->gap_end) {
        // Нужно расширить буфер, если разрыв полностью заполнен
        size_t new_capacity = gb->capacity * 2;
        char* new_buffer = (char*)malloc(new_capacity * sizeof(char));
        memcpy(new_buffer, gb->buffer, gb->gap_start);
        memset(new_buffer + gb->gap_start, 0, new_capacity - gb->capacity);
        memcpy(new_buffer + gb->gap_start + (new_capacity - gb->capacity),
               gb->buffer + gb->gap_end + 1, gb->capacity - gb->gap_end - 1);
        free(gb->buffer);
        gb->buffer = new_buffer;
        gb->gap_end = gb->gap_start + (new_capacity - gb->capacity);
        gb->capacity = new_capacity;
    }

    gb->buffer[gb->gap_start++] = c;
}

void gap_buffer_insert_string(GapBuffer* gb, const char* str) {
    while (*str) {
        gap_buffer_insert(gb, *str++);
    }
}

void gap_buffer_backspace(GapBuffer* gb) {
    if (gb->gap_start > 0) {
        gb->gap_start--;
    }
}

void gap_buffer_delete(GapBuffer* gb) {
    if (gb->gap_end < gb->capacity - 1) {
        gb->gap_end++;
    }
}

void gap_buffer_move_gap(GapBuffer* gb, size_t position) {
    if (position < gb->gap_start) {
        // Перемещение разрыва влево
        while (gb->gap_start > position) {
            gb->buffer[gb->gap_end--] = gb->buffer[--gb->gap_start];
        }
    } else if (position > gb->gap_start) {
        // Перемещение разрыва вправо
        while (gb->gap_start < position) {
            gb->buffer[gb->gap_start++] = gb->buffer[++gb->gap_end];
        }
    }
}

char* gap_buffer_get_content(const GapBuffer* gb) {
    // +1 для нуль-терминатора
    char* content = (char*)malloc(gb->capacity * sizeof(char) + 1);
    if (!content) return NULL;

    size_t content_length = gb->gap_start + (gb->capacity - gb->gap_end - 1);
    memcpy(content, gb->buffer, gb->gap_start);
    memcpy(content + gb->gap_start, gb->buffer + gb->gap_end + 1,
           gb->capacity - gb->gap_end - 1);
    content[content_length] = '\0';  // Добавляем нуль-терминатор

    return content;
}

void gap_buffer_display(const GapBuffer* gb, size_t rows, size_t cols, size_t scroll_pos)
{
    char* content = gap_buffer_get_content(gb);
    if (!content) return;

    size_t line_len = 0;
    size_t num_lines = 0;

    for (size_t i = 0; content[i] != '\0' && num_lines < rows + scroll_pos; ++i) {
        if (content[i] == '\n' || line_len == cols) {
            // Перенос строки или достижение конца колонки
            if (num_lines >= scroll_pos) {
                putchar('\n');
                fflush(stdout);
            }
            line_len = 0;
            num_lines++;
            if (content[i] == '\n') {
                // Если это был символ новой строки, пропускаем следующий символ
                fflush(stdout);
                continue;
            }
            fflush(stdout);
        }

        if (num_lines >= scroll_pos) {
            putchar(content[i]);
            fflush(stdout);
        }
        line_len++;
    }
    fflush(stdout);
    free(content);
}
