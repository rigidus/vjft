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

#include "key_map.h"

#define MAX_BUFFER 1024

char miniBuffer[MAX_BUFFER] = {0};

void addLogLine(const char* text);
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
}

void clearScreen() {
    printf("\033[H\033[J");
}

void moveCursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

/* /\* MiniBuffer *\/ */

/* char miniBuffer[MAX_BUFFER] = {0}; */

/* void updateMiniBuffer(const char* command) { */
/*     snprintf(miniBuffer, sizeof(miniBuffer), "Command: %s", command); */
/* } */


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
    TEXT_INPUT,
    BACKSPACE,
    CTRL,
    SPECIAL,
    DBG
} EventType;

typedef struct InputEvent {
    EventType type;
    char* seq;
    struct InputEvent* next;
} InputEvent;

InputEvent* eventQueue = NULL;

void enqueueEvent(EventType type, char* seq) {
    InputEvent* newEvent = malloc(sizeof(InputEvent));
    newEvent->type = type;
    if (seq) {
        newEvent->seq = malloc(strlen(seq));
        strcpy(newEvent->seq, seq);
    } else {
        newEvent->seq = malloc(strlen("_"));
        strcpy(newEvent->seq, "_");
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
}

// Создание enum с помощью макроса
#define GENERATE_ENUM(val, str) val,
typedef enum {
    KEY_MAP(GENERATE_ENUM)
} Key;

// Создание массива строковых представлений для enum
#define GENERATE_STRING(val, str) str,
const char* key_strings[] = {
    KEY_MAP(GENERATE_STRING)
};

// Создание функции key_to_string
#define GENERATE_KEY_STR(val, str) #val,
const char* key_to_str(Key key) {
    static const char* key_names[] = {
        KEY_MAP(GENERATE_KEY_STR)
    };
    return key_names[key];
}

// Создание функции identify_key
Key identify_key(const char* seq) {
    for (int i = 0; i < sizeof(key_strings) / sizeof(key_strings[0]); i++) {
        if (strcmp(seq, key_strings[i]) == 0) {
            return (Key)i;
        }
    }
    return KEY_UNKNOWN; // Возвращает KEY_UNKNOWN если последовательность не найдена
}

#define ASCII_CODES_BUF_SIZE 127
#define DBG_LOG_MSG_SIZE 255

bool processEvents(char* input, int* input_size, int* cursor_pos,
                   int* log_window_start, int rows, int logSize)
{
    bool updated = false;
    while (eventQueue) {
        InputEvent* event = eventQueue;
        EventType type = event->type;
        eventQueue = eventQueue->next;

        switch(type) {
        /* case SPECIAL: */
        /*     if (event->seq != NULL) { */
        /*     } else { */
        /*         addLogLine("Special key without seq received"); */
        /*     } */
        /*     break; */
        /* case CTRL: */
        /*     char buffer[100]; */
        /*     snprintf(buffer, sizeof(buffer), "CTRL-%s", event->seq); */
        /*     addLogLine(buffer); */
        /*     /\* if (isCtrlStackEmpty()) { *\/ */
        /*     /\*     pushCtrlStack(c); *\/ */
        /*     /\* } else { *\/ */
        /*     /\*     /\\* Повторный ввод 'C-x' - Ошибка ввода команд, очистим CtrlStack *\\/ *\/ */
        /*     /\*     clearCtrlStack(); *\/ */
        /*     /\* } *\/ */
        /*     break; */
        /* case BACKSPACE: */
        /*     printf("\b \b");  // Удаляем символ в терминале */
        /*     if (*input_size > 0 && *cursor_pos > 0) { */
        /*         memmove(&input[*cursor_pos - 1], */
        /*                 &input[*cursor_pos], */
        /*                 *input_size - *cursor_pos); */
        /*         input[--(*input_size)] = '\0'; */
        /*         (*cursor_pos)--; */
        /*         updated = true; */
        /*     } */
        /*     break; */
        /* case TEXT_INPUT: */
        /*     if (event->seq[0] != '\0') { */
        /*         int seq_len = strlen(event->seq); */
        /*         if (isCtrlStackEmpty()) { */
        /*             if (seq_len == 1 && event->seq[0] == '\n') { */
        /*                 // Обрабатываем перевод строки */
        /*                 input[*input_size] = '\0'; // Ensure string is terminated */
        /*                 addLogLine(input); */
        /*                 memset(input, 0, *input_size); */
        /*                 *input_size = 0; */
        /*                 *cursor_pos = 0; */
        /*                 updated = true; */
        /*             } else { */
        /*                 // Обрабатываем обычный символ */
        /*                 if (*input_size + seq_len < MAX_BUFFER - 1) { */
        /*                     memmove(&input[*cursor_pos + seq_len], */
        /*                             &input[*cursor_pos], */
        /*                             *input_size - *cursor_pos); */
        /*                     memcpy(&input[*cursor_pos], event->seq, seq_len); */
        /*                     *input_size += seq_len; */
        /*                     *cursor_pos += seq_len; */
        /*                     updated = true; */
        /*                 } */

        /*             } */
        /*         } else { */
        /*             /\* /\\* Ввод команд *\\/ *\/ */
        /*             /\* if (c == 'p') { *\/ */
        /*             /\*     /\\* SCROLL_UP *\\/ *\/ */
        /*             /\*     (*log_window_start)--; *\/ */
        /*             /\*     if (*log_window_start < 0) { *\/ */
        /*             /\*         *log_window_start = 0; *\/ */
        /*             /\*     } *\/ */
        /*             /\*     updated = true; *\/ */
        /*             /\* } else if (c == 'n') { *\/ */
        /*             /\*     /\\* SCROLL_DOWN: *\\/ *\/ */
        /*             /\*     (*log_window_start)++; *\/ */
        /*             /\*     if (*log_window_start > logSize - (rows - 1)) { *\/ */
        /*             /\*         *log_window_start = logSize - (rows - 1); *\/ */
        /*             /\*     } *\/ */
        /*             /\*     updated = true; *\/ */
        /*             /\* } else { *\/ */
        /*             /\*     /\\* Ошибка, выходим из режима ввода команд *\\/ *\/ */
        /*             /\*     clearCtrlStack(); *\/ */
        /*             /\* } *\/ */
        /*         } */
        /*     } */
        /*     break; */
        case DBG:
            if (event->seq != NULL) {
                int  seq_len = strlen(event->seq);
                char asciiCodes[ASCII_CODES_BUF_SIZE] = {0};
                convertToAsciiCodes(event->seq, asciiCodes, ASCII_CODES_BUF_SIZE);
                char logMsg[DBG_LOG_MSG_SIZE] = {0};

                Key key = identify_key(event->seq);
                if (KEY_UNKNOWN == identify_key) {
                    snprintf(logMsg, sizeof(logMsg), "[DBG]: %s, [%s] <%s>",
                             key_to_str(key), asciiCodes,  event->seq);
                } else {
                    snprintf(logMsg, sizeof(logMsg), "[DBG]: %s, [%s]",
                             key_to_str(key), asciiCodes);
                }
                addLogLine(logMsg);
            }
            break;
        }
        if (event->seq != NULL) {
            free(event->seq);
        }
        free(event);
    }
    return updated;
}


/* LogLine */

typedef struct LogLine {
    char* text;
    struct LogLine* next;
} LogLine;

LogLine* logHead = NULL;
LogLine* logTail = NULL;
int logSize = 0;

void addLogLine(const char* text) {
    LogLine* newLine = malloc(sizeof(LogLine));
    newLine->text = strdup(text);
    newLine->next = NULL;

    if (!logHead) {
        logHead = newLine;
        logTail = newLine;
    } else {
        logTail->next = newLine;
        logTail = newLine;
    }
    logSize++;
}

void freeLog() {
    LogLine* current = logHead;
    while (current) {
        LogLine* next = current->next;
        free(current->text);
        free(current);
        current = next;
    }
}

/* Iface */

void drawHorizontalLine(int cols, int y, char sym) {
    moveCursor(1, y);
    for (int i = 0; i < cols; i++) {
        putchar(sym);
    }
}


int showMiniBuffer(const char* content, int cols, int bottom) {
    int lines = 0;
    int length = strlen(content);
    int line_width = 0;
    drawHorizontalLine(cols, bottom - 1, '=');  // Разделительная линия
    /* printf("\033[%d;1H", bottom-1);  // Перемещение курсора в предпоследнюю строку окна */
    /* printf("\033[2K");  // Очистить строку перед отображением минибуфера */
    /* printf("nmini"); */
    /* for (int i = 0; i < length; ++i) { */
    /*     if (content[i] == '\n' || line_width == window_rows - 1) { */
    /*         if (content[i] == '\n') { */
    /*             putchar(content[i]); */
    /*         } else { */
    /*             printf("\n"); */
    /*         } */
    /*         line_width = 0; */
    /*         lines++; */
    /*         continue; */
    /*     } */
    /*     putchar(content[i]); */
    /*     line_width++; */
    /* } */
    return bottom - 2;
}

int showInputBuffer(char* input, int cursor_pos, int cols, int bottom) {
    // Подсчитываем, сколько строк нужно для отображения input
    int lines = 0;
    int len = strlen(input);
    int line_pos = 0;
    for (int i = 0; i < len; ++i) {
        if ( (input[i] == '\n') || (line_pos >= cols) ) {
            lines++;
            line_pos = 0;
        } else {
            line_pos++;
        }
    }
    // Выводим разделитель
    drawHorizontalLine(cols, bottom - lines - 1, '-');
    // Очищаем на всякий случай всю область вывода
    for (int i = bottom; i < bottom - lines; --i) {
        printf("\033[%d;1H", i);  // Перемещение курсора в строку
        printf("\033[2K");        // Очистить строку
    }

    // Выводим input
    printf("\033[%d;%dH", bottom - lines, 1);
    for (int i = 0; i < strlen(input); ++i) {
        if (input[i] == '\n') {
            putchar('\n');
            moveCursor(1, i);
        } else {
            putchar(input[i]);
        }
    }
    // Сохраняем текущую позицию курсора
    printf("\033[s");
    // Возвращаем новый bottom
    return bottom - lines - 1;
}

// Функция для отображения лога с переносом строк
void showOutputBuffer(int log_window_start, int log_window_end, int cols,
                      int max_lines)
{
    LogLine* current = logHead;
    int lineIndex = 0;
    int displayed_lines = 0;

    // Перемещаемся к началу необходимого диапазона
    while (current && lineIndex < log_window_start) {
        current = current->next;
        lineIndex++;
    }

    // Выводим с начала окна
    moveCursor(1,1);

    // Выводим лог в заданном диапазоне с учетом возможных переносов строк
    while (current && lineIndex < log_window_end) {
        int lineLen = strlen(current->text);
        int printedLength = 0;

        while (printedLength < lineLen && displayed_lines < max_lines) {
            // Если строка или остаток строки короче ширины окна, выводим целиком
            if (lineLen - printedLength <= cols) {
                printf("%s\n", current->text + printedLength);
                printedLength = lineLen;
            } else {
                // Иначе выводим столько символов, сколько помещается в окно,
                /* и делаем перенос */
                fwrite(current->text + printedLength, 1, cols, stdout);
                putchar('\n');
                printedLength += cols;
            }
        }

        current = current->next;
        lineIndex++;
    }
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

// Функция чтения одного UTF-8 символа или ESC последовательности
int read_utf8_char_or_esc_seq(int fd, char* buf, int buf_size) {
    int len = 0, nread;
    while (len < buf_size - 1) {
        nread = read(fd, buf + len, 1);
        if (nread < 0) return -1; // Ошибка чтения
        if (nread == 0) return 0; // Нет данных для чтения
        len++;
        buf[len] = '\0'; // Обеспечиваем нуль-терминирование для строки
        // Проверка на начало escape-последовательности
        if (buf[0] == '\033') {
            // Если считан только первый символ (ESC) - считываем дальше
            if (len == 1) { continue; }
            // Считано два символа и это CSI или SS3 последовательности -
            // считываем дальше
            if (len == 2 && (buf[1] == '[' || buf[1] == 'O')) { continue; }
            // Считано больше двух символов и это CSI или SS3 последовательности
            if (len > 2 && (buf[1] == '[' || buf[1] == 'O')) {
                /* enqueueEvent(DBG, buf+2); */
                // и при этом следующим cчитан алфавитный символ или тильда?
                // - конец последовательности
                if (isalpha(buf[len - 1]) || buf[len - 1] == '~') {
                    break;
                }
                continue;
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
    enqueueEvent(DBG, input_buffer);
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

    bool need_redraw = true;
    int  rows, cols;
    char input[MAX_BUFFER]={0};
    int  input_size = 0;
    int  cursor_pos = 0;  // Позиция курсора в строке ввода
    bool followTail = true; // Флаг (показывать ли последние команды)
    int  log_window_start = 0;

    // Получаем размер терминала
    printf("\033[18t");
    fflush(stdout);
    scanf("\033[8;%d;%dt", &rows, &cols);

    fd_set read_fds;
    struct timeval timeout;

    bool terminate = false;

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
            terminate = keyb();
            // ОБРАБОТКА:
            // Обрабатываем события в конце каждой итерации
            if (processEvents(input, &input_size, &cursor_pos,
                              &log_window_start, rows, logSize)) {
                /* Тут не требуется дополнительных действий, т.к. в начале
                   /* следующего цикла все будет перерисовано */
            }
            // ОТОБРАЖЕНИЕ:
            // Очищаем экран
            clearScreen();
            // Отображаем минибуфер и получаем номер строки над ним
            int bottom = showMiniBuffer(miniBuffer, cols, rows);
            /* Отображаем InputBuffer и получаем номер строки над ним */
            /* NB!: Функция сохраняет позицию курсора с помощью */
            /* escape-последовательности */
            bottom = showInputBuffer(input, cursor_pos, cols, bottom);
            int outputBufferAvailableLines = bottom - 1;
            if (followTail && logSize - outputBufferAvailableLines > 0) {
                // Определяем, с какой строки начать вывод,
                // чтобы показать только последние команды
                if (logSize - outputBufferAvailableLines > 0) {
                    log_window_start = logSize - outputBufferAvailableLines;
                } else {
                    log_window_start = 0;
                }
            }
            // Показываем OutputBuffer в оставшемся пространстве
            showOutputBuffer(log_window_start, logSize, cols,
                             outputBufferAvailableLines);
            // Восстанавливаем сохраненную в функции showInputBuffer позицию курсора
            printf("\033[u");
        }
    }
    freeLog();
    return 0;
}
