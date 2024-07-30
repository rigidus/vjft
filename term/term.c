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

void cmd_forward_word(char* buffer, const char* param) {
    /* // Логика для перемещения вперёд на одно слово */
    /* // Пример простой реализации, предполагая, что input - это строка символов */
    /* while (*cursor_pos < input_size && input[*cursor_pos] != ' ') { */
    /*     (*cursor_pos)++; */
    /* } */
    /* while (*cursor_pos < input_size && input[*cursor_pos] == ' ') { */
    /*     (*cursor_pos)++; */
    /* } */
    /* printf("Cursor moved forward to position: %d\n", *cursor_pos); */
}

void cmd_backward_word(char* buffer, const char* param) {
    /* // Логика для перемещения назад на одно слово */
    /* if (*cursor_pos > 0) (*cursor_pos)--; */
    /* while (*cursor_pos > 0 && input[*cursor_pos] == ' ') { */
    /*     (*cursor_pos)--; */
    /* } */
    /* while (*cursor_pos > 0 && input[*cursor_pos] != ' ') { */
    /*     (*cursor_pos)--; */
    /* } */
    /* printf("Cursor moved backward to position: %d\n", *cursor_pos); */
}

void cmd_insert(char* buffer, const char* param) {
    // Вставка символа или строки из param в buffer
}

KeyMap keyCommands[] = {
    {KEY_ALT_F, "CMD_FORWARD_WORD", cmd_forward_word, NULL},
    {KEY_ALT_B, "CMD_BACKWARD_WORD", cmd_backward_word, NULL},
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

#define ASCII_CODES_BUF_SIZE 127
#define DBG_LOG_MSG_SIZE 255

bool processEvents(GapBuffer* outputBuffer, char* input, int* input_size,
                   int* cursor_pos, int* log_window_start, int rows)
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
                snprintf(logMsg, sizeof(logMsg), "[CMD]: %s\n", event->seq);
                gap_buffer_insert_string(outputBuffer, logMsg);
                updated = true;
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

void calc_display_size(const char* input, int max_width, int* cols, int* rows) {
    *cols = 0;  // Максимальная длина строки в рамках max_width
    *rows = 1;  // Количество строк
    int current_line_length = 0;  // Длина текущей строки

    for (const char* p = input; *p; p++) {
        if (*p == '\n') {
            // Перенос строки по символу новой строки
            if (current_line_length > *cols) {
                *cols = current_line_length;  // Обновление максимальной ширины
            }
            current_line_length = 0;  // Сброс длины строки для новой строки
            (*rows)++;  // Увеличение счетчика строк
        } else {
            current_line_length++;  // Увеличение длины текущей строки
            if (current_line_length == max_width) {
                // Перенос строки по достижению максимальной ширины
                if (current_line_length > *cols) {
                    *cols = current_line_length;  // Обновление максимальной ширины
                }
                current_line_length = 0;  // Сброс длины строки
                (*rows)++;  // Увеличение счетчика строк
            }
        }
    }

    // Обновление cols для последней строки, если она была самой длинной
    if (current_line_length > *cols) {
        *cols = current_line_length;
    }
}


// Вывод текста на экран с автоматическим переносом строк и ограничением высоты
int display_wrapped(const char* text, int x, int y, int max_width, int max_height)
{
    int current_line_length = 0;  // Текущая длина строки
    int current_x = x;            // Текущая позиция курсора X
    int current_y = y;            // Текущая позиция курсора Y
    int line_count = 0;           // Счетчик строк
    moveCursor(current_x, current_y);  // Перемещаем курсор на начальную позицию
    for (const char* p = text; *p; p++) {
        // Если встретили перенос строки или подошли к правой границе
        if (*p == '\n' || current_line_length == max_width) {
            // Заполняем оставшееся пространство пробелами
            while (current_line_length < max_width) {
                putchar(':');
                current_x++;
                current_line_length++;
            }
            // Увеличиваем счетчик строк
            line_count++;
            // Если максимальное количество строк достигнуто
            if (line_count == max_height) {
                break;  // Прекращаем вывод
            }
            current_y++;           // Перемещаемся на одну строку вниз
            current_x = x;         // Возвращаем курсор в начальную позицию X
            moveCursor(current_x, current_y); // Перемещаем физический курсор
            current_line_length = 0; // Обнуляем счетчик длины строки
        } else if (*p != '\n' && line_count < max_height) {
            // Мы здесь, если встреченный символ - это не перенос строки
            // и если до правой границы еще далеко.
            putchar(*p);  // Вывод текущего символа
            current_x++;  // Увеличиваем позицию
            current_line_length++; // Увеличиваем счетчик длины строки
        }
    }
    // Мы вывели все что хотели, но даже после этого мы должны заполнить
    // остаток физической строки до конца
    // Заполняем оставшееся пространство пробелами
    while (current_line_length < max_width) {
        putchar(':');
        current_x++;
        current_line_length++;
    }
    return line_count;
}


void drawHorizontalLine(int cols, int y, char sym) {
    moveCursor(1, y);
    for (int i = 0; i < cols; i++) {
        putchar(sym);
        fflush(stdout);
    }
}


int showMiniBuffer(const char* content, int width, int height) {
    int max_minibuffer_rows = 10;
    int need_cols, need_rows;
    calc_display_size(content, width, &need_cols, &need_rows);
    if (need_rows > max_minibuffer_rows)  {
        need_rows = max_minibuffer_rows;
    }
    int up = height + 1 - need_rows;
    int minibuf_len = display_wrapped(content, 0, up, width, need_rows);
    drawHorizontalLine(width, up-1, '=');  // Разделительная линия
    return up - 2;
}


int showInputBuffer(char* content, int cursor_pos, int width, int height) {
    int max_inputbuffer_rows = height / 2;
    int need_cols, need_rows;
    calc_display_size(content, width, &need_cols, &need_rows);
    if (need_rows > max_inputbuffer_rows)  {
        need_rows = max_inputbuffer_rows;
    }
    int bottom = height - need_rows;
    // Очищаем на всякий случай всю область вывода
    /* for (int i = bottom; i < bottom - lines; --i) { */
    /*     printf("\033[%d;1H", i);  // Перемещение курсора в строку */
    /*     printf("\033[2K");        // Очистить строку */
    /*     fflush(stdout); */
    /* } */
    /* int inputbuf_len = display_wrapped(content, 0, ((display_height + 1) - need_rows), */
    /*                                    display_width, MAX_MINIBUFFER_ROWS); */

    /* // Выводим разделитель */
    /* drawHorizontalLine(display_width, bottom, '-'); */
    /* // Сохраняем текущую позицию курсора */
    /* printf("\033[s"); */
    /* fflush(stdout); */
    /* // Возвращаем новый bottom */
    return bottom - 2;
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
            int rows, int max_width, char* input, int* cursor_pos,
            bool* followTail, int* log_window_start)
{
    // Очищаем экран
    clearScreen();
    // Отображаем минибуфер и получаем номер строки над ним
    char* test_string = "MiniBuffer! This line is definitely longer than twenty-five characters.\nThis is a test of the display size function with a specific max width and height constraint.\nThere is a lot data for small minibuffer";
    int bottom = showMiniBuffer(test_string, max_width, rows);
    /* Отображаем InputBuffer и получаем номер строки над ним */
    /* NB!: Функция сохраняет позицию курсора с помощью */
    /* escape-последовательности */
    bottom = showInputBuffer(test_string, *cursor_pos, max_width, bottom);
    int outputBufferAvailableLines = bottom - 1;
    // Показываем OutputBuffer в оставшемся пространстве
    /* showOutputBuffer(outputBuffer, *log_window_start, logSize, max_width, */
    /*                  outputBufferAvailableLines); */
    showOutputBuffer(outputBuffer, 0, rows, max_width, rows);
    // Flush
    fflush(stdout);
    // Восстанавливаем сохраненную в функции showInputBuffer позицию курсора
    printf("\033[u");
    fflush(stdout);
}


/* Main */

#define READ_TIMEOUT 50000 // 50000 микросекунд (50 миллисекунд)
#define SLEEP_TIMEOUT 100000 // 100 микросекунд

int main() {
    // Отключение буферизации для stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    // Включаем сырой режим
    enableRawMode(STDIN_FILENO);
    // Устанавливаем неблокирующий режим

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
    int  cursor_pos = 0;  // Позиция курсора в строке ввода

    // Получаем размер терминала
    printf("\033[18t");
    fflush(stdout);
    scanf("\033[8;%d;%dt", &rows, &cols);

    fd_set read_fds;
    struct timeval timeout;

    bool terminate = false;

    reDraw(&outputBuffer, rows, cols, input, &cursor_pos,
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
            reDraw(&outputBuffer, rows, cols, input,
                   &cursor_pos, &followTail, &log_window_start);
         }
    }
    gap_buffer_free(&outputBuffer);
    pthread_mutex_destroy(&eventQueue_mutex);
    return 0;
}
