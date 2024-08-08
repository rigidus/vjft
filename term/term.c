#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>
#include <ctype.h>
#include <pthread.h>

#include "key_map.h"
#include "gap-buffer.c"

#define MAX_BUFFER 1024

char miniBuffer[MAX_BUFFER] = {0};

void drawHorizontalLine(int cols, int y, char sym);

/* Term */

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    // Отключаем эхо и канонический режим
    raw.c_lflag &= ~(ECHO | ICANON);
    /* // Отключаем управление потоком */
    /* raw.c_iflag &= ~IXON; */
    /* /\* raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN); *\/ */
    /* /\* raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); *\/ */
    /* /\* raw.c_cflag |= (CS8); *\/ */
    /* /\* raw.c_oflag &= ~(OPOST); *\/ */
    /* // Minimum number of characters for noncanonical read (1) */
    /* raw.c_cc[VMIN] = 0; */
    /* // Timeout in deciseconds for noncanonical read (0.1 seconds) */
    /* raw.c_cc[VTIME] = 0; */
    // Устанавливаем raw mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    // Установка небуферизованного вывода для stdout
    /* setvbuf(stdout, NULL, _IONBF, 0); */
}

void clearScreen() {
    printf("\033[H\033[J");
    fflush(stdout);
}

void moveCursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}


/* CtrlStack */

typedef struct CtrlStack {
    char key;
    struct CtrlStack* next;
} CtrlStack;

CtrlStack* ctrlStack = NULL;

void pushCtrlStack(char key) {
    CtrlStack* newElement = malloc(sizeof(CtrlStack));
    newElement->key = key;
    newElement->next = ctrlStack;
    ctrlStack = newElement;
}

char popCtrlStack() {
    if (ctrlStack == NULL) {
        return '\0';
    }
    char key = ctrlStack->key;
    CtrlStack* temp = ctrlStack;
    ctrlStack = ctrlStack->next;
    free(temp);
    return key;
}

bool isCtrlStackEmpty() {
    return ctrlStack == NULL;
}

void clearCtrlStack() {
    while (!isCtrlStackEmpty()) {
        popCtrlStack();
    }
}


/* InputEvent */

typedef enum {
    DBG,
    CMD
} EventType;

typedef struct InputEvent {
    EventType type;
    char* seq;
    int seq_size;
    struct InputEvent* next;
} InputEvent;

InputEvent* eventQueue = NULL;
pthread_mutex_t eventQueue_mutex = PTHREAD_MUTEX_INITIALIZER;

void enqueueEvent(EventType type, char* seq, int seq_size) {
    pthread_mutex_lock(&eventQueue_mutex);
    InputEvent* newEvent = malloc(sizeof(InputEvent));
    newEvent->type = type;
    if (seq) {
        newEvent->seq = malloc(strlen(seq) + 1);
        strcpy(newEvent->seq, seq);
        newEvent->seq_size = seq_size;
    } else {
        newEvent->seq = malloc(strlen("_") + 1);
        strcpy(newEvent->seq, "_");
        newEvent->seq_size = seq_size;
    }
    newEvent->next = NULL;

    if (!eventQueue) {
        eventQueue = newEvent;
    } else {
        InputEvent* temp = eventQueue;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = newEvent;
    }
    pthread_mutex_unlock(&eventQueue_mutex);
}


// Создание enum с помощью макроса
#define GENERATE_ENUM(val, len, str) val,
typedef enum {
    KEY_MAP(GENERATE_ENUM)
} Key;

// Создание массива строковых представлений для enum
#define GENERATE_STRING(val, len, str) str,
const char* key_strings[] = {
    KEY_MAP(GENERATE_STRING)
};

// Создание функции key_to_string
#define GENERATE_KEY_STR(val, len, str) #val,
const char* key_to_str(Key key) {
    static const char* key_names[] = {
        KEY_MAP(GENERATE_KEY_STR)
    };
    return key_names[key];
}

#define GENERATE_LENGTH(val, len, str) len,
const int key_lengths[] = {
    KEY_MAP(GENERATE_LENGTH)
};


Key identify_key(const char* seq, int seq_size) {
    for (int i = 0; i < sizeof(key_strings) / sizeof(key_strings[0]); i++) {
        if ( (key_lengths[i] == seq_size) &&
             (strncmp(seq, key_strings[i], key_lengths[i]) == 0) ) {
            return (Key)i;
        }
    }
    return KEY_UNKNOWN; // Возвращает KEY_UNKNOWN если последовательность не найдена
}

typedef void (*CommandFunction)(char* buffer, const char* param);

typedef struct {
    Key key;
    char* commandName;
    CommandFunction commandFunc;
    const char* param;
} KeyMap;

/* void cmd_forward_word(char* buffer, const char* param) { */
    /* // Логика для перемещения вперёд на одно слово */
    /* // Пример простой реализации, предполагая, что input - это строка символов */
    /* while (*cursor_pos < input_size && input[*cursor_pos] != ' ') { */
    /*     (*cursor_pos)++; */
    /* } */
    /* while (*cursor_pos < input_size && input[*cursor_pos] == ' ') { */
    /*     (*cursor_pos)++; */
    /* } */
    /* printf("Cursor moved forward to position: %d\n", *cursor_pos); */
/* } */

/* void cmd_backward_word(char* buffer, const char* param) { */
    /* // Логика для перемещения назад на одно слово */
    /* if (*cursor_pos > 0) (*cursor_pos)--; */
    /* while (*cursor_pos > 0 && input[*cursor_pos] == ' ') { */
    /*     (*cursor_pos)--; */
    /* } */
    /* while (*cursor_pos > 0 && input[*cursor_pos] != ' ') { */
    /*     (*cursor_pos)--; */
    /* } */
    /* printf("Cursor moved backward to position: %d\n", *cursor_pos); */
/* } */

char* inputbuffer_text = "What is an input buffer?\nAn input buffer is a temporary storage area used in computing to hold data being received from an input device, such as a keyboard or a mouse. It allows the system to receive and process input at its own pace, rather than being dependent on the speed at which the input is provided.\nHow does an input buffer work?\nWhen you type on a keyboard, for example, the keystrokes are stored in an input buffer until the computer is ready to process them. The buffer holds the keystrokes in the order they were received, allowing them to be processed sequentially. Once the computer is ready, it retrieves the data from the buffer and performs the necessary actions based on the input.\nWhat is the purpose of an input buffer?\nThe main purpose of an input buffer is to decouple the input device from the processing unit of a computer system. By temporarily storing the input data in a buffer, it allows the user to input data at their own pace while the computer processes it independently. This helps to prevent data loss and ensures smooth interaction between the user and the system.\nCan an input buffer be used in programming?\nYes, input buffers are commonly used in programming to handle user input. When writing code, you can create an input buffer to store user input until it is needed for further processing. This allows you to handle user interactions more efficiently and provides a seamless user experience.";

int cursor_pos = 4;  // Позиция курсора в строке ввода

size_t utf8_strlen(const char *str) {
    size_t length = 0;
    unsigned char c;
    while ((c = *str++)) {
        if ((c & 0x80) == 0) {
            // ASCII символ (1 байт) - ничего не пропускаем
        } else if ((c & 0xE0) == 0xC0) {
            str += 1; // 2х байтовый UTF-символ, пропускаем еще 1 байт
        } else if ((c & 0xF0) == 0xE0) {
            str += 2; // 3х байтовый UTF-символ, пропускаем еще 2 байта
        } else if ((c & 0xF8) == 0xF0) {
            str += 3; // 3х байтовый UTF-символ, пропускаем еще 3 байта
        } else {
            // Недопустимый байт - ошибка
            return -1;
        }
        length++;
    }
    return length;
}

void cmd_backward_char() {
    if (cursor_pos-- <= 0) {
        cursor_pos = 0;
    }
}

void cmd_forward_char() {
    int len = utf8_strlen(inputbuffer_text);
    if (cursor_pos++ >= len) {
        cursor_pos = len;
    }
}

void cmd_insert(char* buffer, const char* param) {
    // Вставка символа или строки из param в buffer
}

KeyMap keyCommands[] = {
    {KEY_CTRL_B, "CMD_BACKWARD_CHAR", cmd_backward_char, NULL},
    {KEY_CTRL_F, "CMD_FORWARD_CHAR", cmd_forward_char, NULL},
    /* {KEY_ALT_F, "CMD_FORWARD_WORD", cmd_forward_word, NULL}, */
    /* {KEY_ALT_B, "CMD_BACKWARD_WORD", cmd_backward_word, NULL}, */
    {KEY_A, "CMD_INSERT", cmd_insert, "a"},
};

const KeyMap* findCommandByKey(Key key) {
    int n_commands = sizeof(keyCommands) / sizeof(keyCommands[0]);
    for (int i = 0; i < n_commands; i++) {
        if (keyCommands[i].key == key) {
            return &keyCommands[i];
        }
    }
    return NULL;
}


void convertToAsciiCodes(const char *input, char *output, size_t outputSize) {
    size_t inputLen = strlen(input);
    size_t offset = 0;

    for (size_t i = 0; i < inputLen; i++) {
        int written = snprintf(output + offset, outputSize - offset, "%x", (unsigned char)input[i]);
        if (written < 0 || written >= outputSize - offset) {
            // Output buffer is full or an error occurred
            break;
        }
        offset += written;
        if (i < inputLen - 1) {
            // Add a space between numbers, but not after the last number
            if (offset < outputSize - 1) {
                output[offset] = ' ';
                offset++;
            } else {
                // Output buffer is full
                break;
            }
        }
    }
    output[offset] = '\0'; // Null-terminate the output string
}

#define ASCII_CODES_BUF_SIZE 127
#define DBG_LOG_MSG_SIZE 255

bool processEvents(GapBuffer* outputBuffer, char* input, int* input_size,
                   const int* cursor_pos, int* log_window_start, int rows)
{
    pthread_mutex_lock(&eventQueue_mutex);
    bool updated = false;
    while (eventQueue) {
        InputEvent* event = eventQueue;
        EventType type = event->type;
        eventQueue = eventQueue->next;

        switch(type) {
        case DBG:
            if (event->seq != NULL) {
                char asciiCodes[ASCII_CODES_BUF_SIZE] = {0};
                convertToAsciiCodes(event->seq, asciiCodes, ASCII_CODES_BUF_SIZE);
                Key key = identify_key(event->seq, event->seq_size);
                char logMsg[DBG_LOG_MSG_SIZE] = {0};
                snprintf(logMsg, sizeof(logMsg), "[DBG]: %s, [%s] (%d)\n",
                         key_to_str(key), asciiCodes, event->seq_size);
                gap_buffer_insert_string(outputBuffer, logMsg);
                updated = true;
            }
            break;
        case CMD:
            if (event->seq != NULL) {
                char logMsg[DBG_LOG_MSG_SIZE] = {0};
                /* snprintf(logMsg, sizeof(logMsg), "[CMD]: %s\n", event->seq); */
                /* gap_buffer_insert_string(outputBuffer, logMsg); */

                const KeyMap* command = NULL;
                int n_commands = sizeof(keyCommands) / sizeof(keyCommands[0]);
                for (int i = 0; i < n_commands; i++) {
                    if (strcmp(keyCommands[i].commandName, event->seq) == 0) {
                        command = &keyCommands[i];
                        break;
                    }
                }
                if (command) {
                    command->commandFunc(inputbuffer_text, command->param);
                    /* snprintf(logMsg, sizeof(logMsg), */
                             /* "Executing command: %s\n", command->commandName); */
                    /* gap_buffer_insert_string(outputBuffer, logMsg); */
                } else {
                    snprintf(logMsg, sizeof(logMsg),
                             "No command found for: %s\n", event->seq);
                    gap_buffer_insert_string(outputBuffer, logMsg);
                }
               /* updated = true; */
            }
            break;
        }
        if (event->seq != NULL) {
            free(event->seq);
        }
        free(event);
    }
    pthread_mutex_unlock(&eventQueue_mutex);
    return updated;
}


/* Iface */

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

    for (const char* p = text; *p; p++) {
        cur_text_pos++; // Увеличение текущей позиции в тексте
        if (cur_text_pos == cursor_pos) { // Дошли до позиции курсора?
            *cursor_row = cur_row;        // Вернуть текущую строку
            *cursor_col = cur_col;        // Вернуть текущий столбец
        }

        cur_col++; // Увеличение позиции в текущей строке
        // Если эта строка длиннее чем все ранее встреченные
        if (cur_col > max_col) {
            max_col = cur_col; // Обновить
        }

        // Символ переноса строки или правая граница?
        if ((*p == '\n') || (cur_col >= max_width)) {
            cur_col = 0;  // Сброс длины строки для новой строки
            cur_row++;    // Увеличение счетчика строк
        }
    }

    // Если последняя строка оказалась самой длинной
    if (cur_col > max_col) {
        max_col = cur_col;
    }

    *need_rows = cur_row;
    *need_cols = max_col;
}


/**
   Вывод текста в текстовое окно экрана с переносом строк
   Вывод начинается с from_row строки и выводится max_width строк
*/

#define FILLER ':'
void display_wrapped(const char* text, int x, int y,
                     int max_width, int max_rows,
                     int from_row)
{
    int cursor_x = x; // Текущая позиция курсора X
    int cursor_y = y; // Текущая позиция курсора Y
    int cur_row = 0;  // Текущая строка, она же счетчик строк
    int cur_col = 0;  // Текущий столбец

    bool is_not_skipped_row() {
        return (cur_row > (from_row + 0));
    }
    void fullfiller () {
        if (is_not_skipped_row()) { // Если мы не пропускаем
            while (cur_col < max_width) { // Пока не правая граница
                putchar(FILLER); // Заполняем
                cur_col++; // Увеличиваем счетчик длины строки
            }
        }
    }
    void inc_cur_row() {
        cur_col = 0; // Сброс длины строки для новой строки
        cur_row++;   // Увеличение счетчика строк
        if (is_not_skipped_row()) { // Если мы не пропускаем
            moveCursor(cursor_x, cursor_y + cur_row - from_row - 1);
        }
    }

    inc_cur_row(); // Курсор на начальную позицию

    for (const char* p = text; *p; p++) {
        if (cur_col >= max_width) { // Если правая граница
            inc_cur_row();
        }
        // Если максимальное количество строк достигнуто
        if (cur_row > (max_rows + from_row)) {
            break;  // Прекращаем вывод
        }
        if (*p == '\n') {  // Если текущий символ - перевод строки
            fullfiller();  // Заполняем оставшееся филером
            inc_cur_row(); // Переходим на следующую строку
        } else {
            // Обычный печатаемый символ
            if (is_not_skipped_row()) { // Если мы не пропускаем
                putchar(*p); // то выводим текущий символ
            }
            cur_col++; // Увеличиваем счетчик длины строки
        }
    }

    // Мы вывели все что хотели, но даже после этого мы должны заполнить
    // остаток строки до конца, если мы не находимся в начале строки
    if (cur_col != 0) {
        fullfiller();
    }
}


void drawHorizontalLine(int cols, int y, char sym) {
    moveCursor(1, y);
    for (int i = 0; i < cols; i++) {
        putchar(sym);
        fflush(stdout);
    }
}


int showMiniBuffer(const char* text, int width, int height) {
    int need_cols, need_rows, cursor_row, cursor_col;
    calc_display_size(text, width-2, 0,
                      &need_cols, &need_rows,
                      &cursor_row, &cursor_col);
    // Когда я вывожу что-то в минибуфер я хочу чтобы при большом
    // выводе он показывал только первые 10 строк
    int max_minibuffer_rows = 10;
    if (need_rows > max_minibuffer_rows)  {
        need_rows = max_minibuffer_rows;
    }
    int from_row = 0;
    int up = height + 1 - need_rows + from_row;
    display_wrapped(text, 2, up, width-2, need_rows, from_row);
    drawHorizontalLine(width, up-1, '=');
    return up - 2;
}


int showInputBuffer(char* text, int cursor_pos,
                    int width, int height)
{
    // Получаем размеры виртуальной области вывода, т.е. сколько нужно
    // места, чтобы вывести весь text при заданной ширине строки и в
    // какой из этих строк будет расположен курсор
    int need_cols, need_rows, cursor_row, cursor_col;
    calc_display_size(text, width-2, cursor_pos,
                      &need_cols, &need_rows,
                      &cursor_row, &cursor_col);
    // Если содержимое не помещается в область вывода, которая может
    // быть максимум половиной от оставшейся высоты
    int max_inputbuffer_rows = height / 2;
    if (need_rows > max_inputbuffer_rows)  {
        // Тогда надо вывести область где курсор
        need_rows = max_inputbuffer_rows;
    }
    // С какой строки выводим
    int from_row = 0;
    // Определяем координаты верхней строки
    int up = height + 1 - need_rows;
    // Выводим
    display_wrapped(text, 2, up, width-2, need_rows, from_row);
    drawHorizontalLine(width, up-1, '-');
    // Возвращаем номер строки выше отображения inputbuffer
    return up - 2;
}

void showOutputBuffer(GapBuffer* gb, int log_window_start, int log_window_end,
                      int cols, int max_lines)
{
    // Получаем содержимое GAP-буфера как единую строку
    char* content = gap_buffer_get_content(gb);
    if (!content) return;

    int lineIndex = 0;  // Индекс текущей строки
    int displayed_lines = 0;  // Количество уже отображенных строк

    // Перемещение к началу нужного диапазона
    char* current_pos = content;
    while (*current_pos && lineIndex < log_window_start) {
        if (*current_pos == '\n') lineIndex++;
        current_pos++;
    }

    // Вывод с начала окна
    moveCursor(1, 1);

    int line_len = 0;
    char* line_start = current_pos;
    while (*current_pos && lineIndex < log_window_end && displayed_lines < max_lines) {
        if (*current_pos == '\n' || line_len >= cols) {
            // Если строка или остаток строки короче ширины окна, выводим целиком
            if (line_len < cols) {
                printf("%.*s\n", line_len, line_start);
                fflush(stdout);
            } else {
                // Иначе выводим столько символов, сколько помещается в окно,
                // и делаем перенос
                fwrite(line_start, 1, cols, stdout);
                putchar('\n');
                fflush(stdout);
            }
            line_start = current_pos + 1;  // Перемещаем начало следующей строки
            line_len = 0;
            lineIndex++;
            displayed_lines++;
        }
        line_len++;
        current_pos++;
    }
    fflush(stdout);
    free(content);
}


/* UTF-8 */

// Проверка, что UTF-8 символ полностью считан
bool is_utf8_complete(const char* buffer, int len) {
    if (len == 0) return false;
    int expected_len = 0;
    unsigned char c = buffer[0];
    if ((c & 0x80) == 0) expected_len = 1;          // 0xxxxxxx
    else if ((c & 0xE0) == 0xC0) expected_len = 2;  // 110xxxxx
    else if ((c & 0xF0) == 0xE0) expected_len = 3;  // 1110xxxx
    else if ((c & 0xF8) == 0xF0) expected_len = 4;  // 11110xxx
    return expected_len == len;
}

// Функция чтения одного UTF-8 символа или ESC последовательности в buf
int read_utf8_char_or_esc_seq(int fd, char* buf, int buf_size) {
    int len = 0, nread;
    struct timeval timeout;
    fd_set read_fds;
    while (len < buf_size - 1) {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        // Сброс таймаута на каждой итерации
        timeout.tv_sec = 0; // 100 мс только после ESC
        timeout.tv_usec = (len == 1 && buf[0] == '\033') ? 100000 : 0;
        // Читаем
        nread = read(fd, buf + len, 1);
        if (nread < 0) return -1; // Ошибка чтения
        if (nread == 0) return 0; // Нет данных для чтения
        // Увеличиваем len и обеспечиваем нуль-терминирование для строки
        len++;
        buf[len] = '\0';
        // Проверка на начало escape-последовательности
        if (buf[0] == '\033') {
            // Если считан только первый символ и это ESC
            if (len == 1) {
                // Ждем, чтобы убедиться, что за ESC ничего не следует
                int select_res = select(fd + 1, &read_fds, NULL, NULL, &timeout);
                if (select_res == 0) {
                    // Тайм-аут истек, и других символов не было получено
                    break; // Только ESC был нажат
                }
                continue; // Есть еще данные для чтения
            }
            // Считано два символа и это CSI или SS3 последовательности - читаем дальше
            if (len == 2 && (buf[1] == '[' || buf[1] == 'O')) { continue; }
            // Считано два символа, и второй из них - это \xd0 или \xd1 - читаем дальше
            // (это может быть модификатор + двухбайтовый кирилический символ)
            if (len == 2 && (buf[1] == '\xd0' || buf[1] == '\xd1')) { continue; }
            // Считано больше двух символов и это CSI или SS3 последовательности
            if (len > 2 && (buf[1] == '[' || buf[1] == 'O')) {
                // ...и при этом следующим cчитан алфавитный символ или тильда?
                if (isalpha(buf[len - 1]) || buf[len - 1] == '~') {
                    // последовательность завершена
                    break;
                } else {
                    // продолжаем считывать дальше
                    continue;
                }
            }
            // Тут мы окажемся только если после ESC пришло что-то неожиданное
            break;
        } else {
            // Это не escape-последовательность, а UTF-8 символ, который мы должны
            // прекратить читать только когда он будет complete
            if (is_utf8_complete(buf, len)) break;
        }
    }
    return len;
}

#define MAX_INPUT_BUFFER 40

bool keyb () {
    char input_buffer[MAX_INPUT_BUFFER];
    int len = read_utf8_char_or_esc_seq(
        STDIN_FILENO, input_buffer, sizeof(input_buffer));

    Key key = identify_key(input_buffer, len);
    const KeyMap* command = findCommandByKey(key);
    if (command) {
        enqueueEvent(CMD, strdup(command->commandName), 1 /* command->param */);
    } else {
        enqueueEvent(DBG, input_buffer, len);
    }

    if (len == 1) {
        // SingleByte
        if (input_buffer[0] == '\x04') {
            // Обрабатываем Ctrl-D для выхода
            printf("\n");
            return true;
            /* } else if (input_buffer[0] == 127 || input_buffer[0] == 8) { */
            /*     // Обрабатываем BackSpace */
            /*     enqueueEvent(BACKSPACE, NULL); */
            /* } else if (input_buffer[0] >= 1 && input_buffer[0] <= 26 ) { */
            /*     // Обрабатываем Ctrl-key для команд */
            /*     input_buffer[0] = 'A' + input_buffer[0] - 1; */
            /*     enqueueEvent(CTRL, input_buffer); */
            /* } else if (input_buffer[0] == '\033') { */
            /*     // Escape-последовательность */
            /*     enqueueEvent(SPECIAL, input_buffer); */
            /* } else { */
            /*     // Обычный однобайтовый символ - выводим */
            /*     enqueueEvent(TEXT_INPUT, input_buffer); */
        }
    } else {
        /* // Multibyte */
        /* if (input_buffer[0] == '\033') { */
        /*     // Escape-последовательность */
        /*     char buffer[100]; */
        /*     snprintf(buffer, sizeof(buffer), "ESC:%d <%s>", len, */
        /*              input_buffer+1); */
        /*     // Отладочный вывод */
        /*     enqueueEvent(DBG, buffer); */
        /*     // Escape-последовательность - выводим */
        /*     enqueueEvent(SPECIAL, input_buffer+1); */
        /* } else { */
        /*     // Многобайтовый символ */
        /*     char buffer[100]; */
        /*     snprintf(buffer, sizeof(buffer), "MulitByteSym: %d [%s]", len, */
        /*              input_buffer); */
        /*     // Отладочный вывод */
        /*     enqueueEvent(DBG, buffer); */
        /*     // Обычный многобайтовый символ */
        /*     enqueueEvent(TEXT_INPUT, input_buffer); */
        /* } */
    }
}


void reDraw(GapBuffer* outputBuffer,
            char* inputbuffer_text,
            int rows, int max_width, char* input, const int* cursor_pos,
            bool* followTail, int* log_window_start)
{
    int need_cols, need_rows;

    // Вычисляемая на основании cursor_pos (позиции курсора в строке)
    // реальная позиция курсора на экране
    int cursor_row = 0;
    int cursor_col = 0;

    // Вычисляем необходимый размер inputbuffer и позицию курсора на экране
    calc_display_size(inputbuffer_text, max_width-2, *cursor_pos,
                      &need_cols, &need_rows, &cursor_row, &cursor_col);

    // Формируем текст для минибуфера
    char minibuffer_text[1024] = {0};
    snprintf(minibuffer_text, 1024, "cursor_pos=%d cursor_row=%d cursor_col=%d",
             *cursor_pos, cursor_row, cursor_col);

    // Очищаем экран
    clearScreen();
    // Отображаем минибуфер и получаем номер строки над ним
    int bottom = showMiniBuffer(minibuffer_text, max_width, rows);
    // Отображаем InputBuffer и получаем номер строки над ним
    bottom = showInputBuffer(inputbuffer_text, *cursor_pos, max_width, bottom);
    int outputBufferAvailableLines = bottom - 1;
    // Показываем OutputBuffer в оставшемся пространстве
    /* showOutputBuffer(outputBuffer, *log_window_start, logSize, max_width, */
    /*                  outputBufferAvailableLines); */
    showOutputBuffer(outputBuffer, 0, rows, max_width, rows);
    // Flush
    fflush(stdout);

    // Перемещаем курсор в нужную позицию с поправкой на расположение inputbuffer
    moveCursor(cursor_col + 1, bottom + 1 + cursor_row);
}


/* Main */

#define READ_TIMEOUT 50000 // 50000 микросекунд (50 миллисекунд)
#define SLEEP_TIMEOUT 100000 // 100 микросекунд

int main() {
    // Отключение буферизации для stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    // Включаем сырой режим
    enableRawMode(STDIN_FILENO);

    char nc;
    char input[MAX_BUFFER]={0};
    int  input_size = 0;
    int  log_window_start = 0;
    GapBuffer outputBuffer;
    size_t initialCapacity = 1024;
    gap_buffer_init(&outputBuffer, initialCapacity);

    // Вставка тестовых строк в GAP-буфер
    const char* testStrings[] = {
        "Hello, World!\n",
        "This is a test of the GAP buffer system.\n",
    };
    for (int i = 0; i < 2; i++) {
        gap_buffer_insert_string(&outputBuffer, testStrings[i]);
    }

    int  rows, cols;
    bool need_redraw = true;
    bool followTail = true; // Флаг (показывать ли последние команды)

    // Получаем размер терминала
    printf("\033[18t");
    fflush(stdout);
    scanf("\033[8;%d;%dt", &rows, &cols);

    fd_set read_fds;
    struct timeval timeout;

    bool terminate = false;

    reDraw(&outputBuffer,
           inputbuffer_text,
           rows, cols, input, &cursor_pos,
           &followTail, &log_window_start);
    while (!terminate) {
        // Переинициализация структур для select
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        // Устанавливаем время ожидания
        timeout.tv_sec = 0;
        timeout.tv_usec = READ_TIMEOUT;
        // Вызываем Select
        int select_result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result = 0) {
            // Обработка таймаута
            usleep(SLEEP_TIMEOUT);
        } else if (select_result < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        } else { // select_result > 0
            // ПОЛУЧЕНИЕ ВВОДА:
            terminate = keyb();
            // ОБРАБОТКА:
            // Обрабатываем события в конце каждой итерации
            if (processEvents(&outputBuffer, input, &input_size, &cursor_pos,
                              &log_window_start, rows)) {
                /* Тут не требуется дополнительных действий, т.к. в начале */
                /* следующего цикла все будет перерисовано */
            }
            // ОТОБРАЖЕНИЕ:
            reDraw(&outputBuffer,
                   inputbuffer_text,
                   rows, cols, input,
                   &cursor_pos, &followTail, &log_window_start);
        }
    }
    gap_buffer_free(&outputBuffer);
    pthread_mutex_destroy(&eventQueue_mutex);
    return 0;
}
