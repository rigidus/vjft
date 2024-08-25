//term.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "term.h"

#define MAX_BUFFER 1024

char miniBuffer[MAX_BUFFER] = {0};

void drawHorizontalLine(int cols, int y, char sym);

/* Term */

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    // Сохраняем текущие настройки терминала
    tcgetattr(STDIN_FILENO, &orig_termios);
    // Гарантируем возврат к нормальным настройкам при выходе
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    // Отключаем эхо и канонический режим
    raw.c_lflag &= ~(ECHO | ICANON);

    // Отключаем эхо, канонический режим и сигналы
    /* raw.c_lflag &= ~(ECHO | ICANON | ISIG); */

    // Отключаем специальные управляющие символы
    /* raw.c_cc[VINTR] = 0;  // Ctrl-C */
    /* raw.c_cc[VQUIT] = 0;  // Ctrl-\ ; */
    /* raw.c_cc[VSTART] = 0; // Ctrl-Q */
    /* raw.c_cc[VSTOP] = 0;  // Ctrl-S */
    /* raw.c_cc[VEOF] = 0;   // Ctrl-D */
    /* raw.c_cc[VSUSP] = 0;  // Ctrl-Z */
    /* raw.c_cc[VREPRINT] = 0; // Ctrl-R */
    /* raw.c_cc[VLNEXT] = 0; // Ctrl-V */
    /* raw.c_cc[VDISCARD] = 0; // Ctrl-U */

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

volatile int win_cols = 0;
volatile int win_rows = 0;

void upd_win_size() {
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0)
    {
        win_cols = size.ws_col;
        win_rows = size.ws_row;
    }
}

/* List of Messages */

typedef struct MessageNode {
    char* message;
    struct MessageNode* prev;
    struct MessageNode* next;
    int cursor_pos;
} MessageNode;

typedef struct {
    MessageNode* head;
    MessageNode* tail;
    MessageNode* current;
    int size;
} MessageList;

pthread_mutex_t messageList_mutex = PTHREAD_MUTEX_INITIALIZER;
MessageList messageList = {0};

void initMessageList(MessageList* list) {
    *list = (MessageList){0};
}

void pushMessage(MessageList* list, const char* text) {
    pthread_mutex_lock(&messageList_mutex);

    MessageNode* newNode = (MessageNode*)malloc(sizeof(MessageNode));
    if (newNode == NULL) {
        perror("Failed to allocate memory for new message node");
        pthread_mutex_unlock(&messageList_mutex);
        exit(1);
    }

    newNode->message = strdup(text);
    if (newNode->message == NULL) {
        free(newNode);
        perror("Failed to duplicate message string");
        pthread_mutex_unlock(&messageList_mutex);
        return;
    }

    newNode->prev = NULL;
    newNode->next = list->head;
    newNode->cursor_pos = 0;

    if (list->head) {
        list->head->prev = newNode;
    } else {
        list->tail = newNode;
    }

    list->head = newNode;

    if (list->current == NULL) {
        list->current = newNode;
    }

    list->size++;

    pthread_mutex_unlock(&messageList_mutex);
}

void clearMessageList(MessageList* list) {
    MessageNode* current = list->head;
    while (current != NULL) {
        MessageNode* temp = current;
        current = current->next;
        free(temp->message);  // Освобождение памяти выделенной под строку
        free(temp);           // Освобождение памяти выделенной под узел
    }
    list->head = NULL;
    list->tail = NULL;
    list->current = NULL;
    list->size = 0;
}

void moveToNextMessage(MessageList* list) {
    /* pthread_mutex_lock(&messageList_mutex); */
    if (list->current && list->current->next) {
        list->current = list->current->next;
    }
    /* pthread_mutex_unlock(&messageList_mutex); */
}

void moveToPreviousMessage(MessageList* list) {
    /* pthread_mutex_lock(&messageList_mutex); */
    if (list->current && list->current->prev) {
        list->current = list->current->prev;
    }
    /* pthread_mutex_unlock(&messageList_mutex); */
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
    /* ctrlStack = &(CtrlStack){.key = key, .next = ctrlStack}; */
}

char popCtrlStack() {
    if (!ctrlStack) return '\0';
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
    while (popCtrlStack());
}


/* InputEvent */

typedef enum {
    DBG,
    CMD
} EventType;

typedef void (*CmdFunc)(MessageNode*, const char* param);

typedef struct InputEvent {
    EventType type;
    CmdFunc cmdFn;
    char* seq;
    int seq_size;
    struct InputEvent* next;
} InputEvent;

InputEvent* eventQueue = NULL;
pthread_mutex_t eventQueue_mutex = PTHREAD_MUTEX_INITIALIZER;

void enqueueEvent(EventType type, CmdFunc cmdFn, char* seq, int seq_size) {
    pthread_mutex_lock(&eventQueue_mutex);

    InputEvent* newEvent = malloc(sizeof(InputEvent));
    if (newEvent == NULL) {
        perror("Failed to allocate memory for a new event");
        pthread_mutex_unlock(&eventQueue_mutex);
        exit(1); // Выход при ошибке выделения памяти
    }

    *newEvent  = (InputEvent){
        .type  = type,
        .cmdFn = cmdFn,
        .seq   = seq ? strdup(seq) : strdup("_"),
        .seq_size = seq_size,
        .next  = NULL
    };

    if (eventQueue == NULL) {
        eventQueue = newEvent;
    } else {
        InputEvent* lastEvent = eventQueue;
        while (lastEvent->next) { lastEvent = lastEvent->next; }
        lastEvent->next = newEvent;
    }

    pthread_mutex_unlock(&eventQueue_mutex);
}


// Создание enum с помощью макроса
#define GENERATE_ENUM(val, len, str, is_printable) val,
typedef enum {
    KEY_MAP(GENERATE_ENUM)
} Key;

// Создание массива строковых представлений для enum
#define GENERATE_STRING(val, len, str, is_printable) str,
const char* key_strings[] = {
    KEY_MAP(GENERATE_STRING)
};

// Создание функции key_to_string
#define GENERATE_KEY_STR(val, len, str, is_printable) #val,
const char* key_to_str(Key key) {
    static const char* key_names[] = {
        KEY_MAP(GENERATE_KEY_STR)
    };
    return key_names[key];
}

#define GENERATE_LENGTH(val, len, str, is_printable) len,
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
    return KEY_UNKNOWN; // return KEY_UNKNOWN if seq not found
}

// Функция для вычисления длины UTF-8 символа
size_t utf8_char_length(const char* c) {
    unsigned char byte = (unsigned char)*c;
    if (byte <= 0x7F) return 1;        // ASCII
    else if ((byte & 0xE0) == 0xC0) return 2; // 2-byte sequence
    else if ((byte & 0xF0) == 0xE0) return 3; // 3-byte sequence
    else if ((byte & 0xF8) == 0xF0) return 4; // 4-byte sequence
    return 1; // Ошибочные символы обрабатываем как 1 байт
}

// Возвращает длину строки в utf-8 символах
size_t utf8_strlen(const char *str) {
    size_t length = 0;
    for (; *str; length++) {
        str += utf8_char_length(str);
    }
    return length;
}

void cmd_backward_char(MessageNode* node, const char* stub) {
    if (node->cursor_pos > 0) { node->cursor_pos--; }
}

void cmd_forward_char(MessageNode* node, const char* stub) {
    int len = utf8_strlen(node->message);
    if (++node->cursor_pos > len) { node->cursor_pos = len; }
}

// Функция для перемещения курсора на следующий UTF-8 символ
int utf8_next_char(const char* str, int pos) {
    if (str[pos] == '\0') return pos;
    return pos + utf8_char_length(&str[pos]);
}

// Функция для перемещения курсора на предыдущий UTF-8 символ
int utf8_prev_char(const char* str, int pos) {
    if (pos == 0) return 0;
    do {
        pos--;
    } while (pos > 0 && ((str[pos] & 0xC0) == 0x80)); // Пропуск байтов продолжения
    return pos;
}

// Функция для определения смещения в байтах
// для указанной позиции курсора (в символах)
int utf8_byte_offset(const char* str, int cursor_pos) {
    int byte_offset = 0, char_count = 0;
    while (str[byte_offset] && char_count < cursor_pos) {
        byte_offset += utf8_char_length(&str[byte_offset]);
        char_count++;
    }
    return byte_offset;
}

// Функция для конвертации byte_offset в индекс символа (UTF-8)
int utf8_char_index(const char* str, int byte_offset) {
    int char_index = 0;
    int current_offset = 0;

    while (current_offset < byte_offset && str[current_offset] != '\0') {
        int char_len = utf8_char_length(&str[current_offset]);
        current_offset += char_len;
        char_index++;
    }

    return char_index;
}

// Перемещение курсора вперед на одно слово
void cmd_forward_word(MessageNode* node, const char* stub) {
    int len = utf8_strlen(node->message);  // Длина строки в байтах

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Пропуск пробелов (если курсор находится на пробеле)
    while (byte_offset < len && isspace((unsigned char)node->message[byte_offset]))
    {
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }

    // Перемещение вперед до конца слова
    while (byte_offset < len && !isspace((unsigned char)node->message[byte_offset]))
    {
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }
}

// Перемещение курсора назад на одно слово
void cmd_backward_word(MessageNode* node, const char* stub) {
    if (node->cursor_pos == 0) return;  // Если курсор уже в начале, ничего не делаем

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Пропуск пробелов, если курсор находится на пробеле
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->message, byte_offset);
        if (!isspace((unsigned char)node->message[prev_offset])) {
            break;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }

    // Перемещение назад до начала предыдущего слова
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->message, byte_offset);
        if (isspace((unsigned char)node->message[prev_offset])) {
            break;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }
}

/* void cmd_move_to_beginning_of_line(MessageNode* node, const char* stub) { */
/*     // Если курсор уже в начале текста, ничего не делаем */
/*     if (node->cursor_pos == 0) return; */

/*     // Смещение в байтах от начала строки до курсора */
/*     int byte_offset = utf8_byte_offset(node->message, node->cursor_pos); */

/*     // Движемся назад, пока не найдем начало строки или начало текста */
/*     while (byte_offset > 0) { */
/*         int prev_offset = utf8_prev_char(node->message, byte_offset); */
/*         if (node->message[prev_offset] == '\n') { */
/*             // Переходим на следующий символ после \n */
/*             byte_offset = utf8_next_char(node->message, prev_offset); */
/*             node->cursor_pos = utf8_strlen(node->message); */
/*             return; */
/*         } */
/*         byte_offset = prev_offset; */
/*         node->cursor_pos--; */
/*     } */
/*     // Если достигли начала текста, устанавливаем курсор на позицию 0 */
/*     node->cursor_pos = 0; */
/* } */

void cmd_move_to_end_of_line(MessageNode* node, const char* stub) {
    // Длина строки в байтах
    int len = strlen(node->message);

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Движемся вперед, пока не найдем конец строки или конец текста
    while (byte_offset < len) {
        if (node->message[byte_offset] == '\n') {
            node->cursor_pos = utf8_char_index(node->message, byte_offset);
            return;
        }
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }

    // Если достигли конца текста
    node->cursor_pos = utf8_char_index(node->message, len);
}

void cmd_next_msg() {
    moveToNextMessage(&messageList);
}

void cmd_prev_msg() {
    moveToPreviousMessage(&messageList);
}


// Функция для вставки текста в позицию курсора
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void cmd_insert(MessageNode* node, const char* insert_text) {
    pthread_mutex_lock(&lock);
    if (!node || !node->message) return;

    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);
    int insert_len = strlen(insert_text);

    char* new_message = realloc(node->message, strlen(node->message) + insert_len + 1);
    if (new_message == NULL) {
        perror("Failed to reallocate memory");
        exit(1);
    }
    node->message = new_message;

    memmove(node->message + byte_offset + insert_len,
            node->message + byte_offset,
            strlen(node->message) - byte_offset + 1);
    memcpy(node->message + byte_offset,
           insert_text,
           insert_len);

    node->cursor_pos += utf8_strlen(insert_text);
    pthread_mutex_unlock(&lock);
}



/* Commands */

typedef struct {
    Key key;
    char* cmdName;
    CmdFunc cmdFunc;
    char* param;
} KeyMap;

KeyMap keyCommands[] = {
    {KEY_CTRL_P, "CMD_PREV_MSG", cmd_prev_msg, NULL},
    {KEY_CTRL_N, "CMD_NEXT_MSG", cmd_next_msg, NULL},
    {KEY_CTRL_B, "CMD_BACKWARD_CHAR", cmd_backward_char, NULL},
    {KEY_CTRL_F, "CMD_FORWARD_CHAR", cmd_forward_char, NULL},
    {KEY_ALT_F, "CMD_FORWARD_WORD", cmd_forward_word, NULL},
    {KEY_ALT_B, "CMD_BACKWARD_WORD", cmd_backward_word, NULL},
    /* {KEY_CTRL_A, "CMD_MOVE_TO_BEGINNING_OF_LINE",
       cmd_move_to_beginning_of_line, NULL}, */
    {KEY_CTRL_E, "CMD_MOVE_TO_END_OF_LINE", cmd_move_to_end_of_line, NULL},
    {KEY_K, "CMD_INSERT_K", cmd_insert, "=ey"},
    {KEY_L, "CMD_INSERT_L", cmd_insert, "-ol"},
};


const KeyMap* findCommandByKey(Key key) {
    for (int i = 0; i < sizeof(keyCommands) / sizeof(KeyMap); i++) {
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

bool processEvents(char* input, int* input_size,
                   int* log_window_start, int rows)
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
                pushMessage(&messageList, logMsg);
                updated = true;
            }
            break;
        case CMD:
            if (event->cmdFn) {
                event->cmdFn(messageList.current, event->seq);
                updated = true;
            } else {
                char logMsg[DBG_LOG_MSG_SIZE] = {0};
                snprintf(logMsg, sizeof(logMsg),
                         "case CMD: No command found for: %s\n", event->seq);
                pushMessage(&messageList, logMsg);
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

    for (const char* p = text; *p; ) {
        size_t char_len = utf8_char_length(p);

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

        p += char_len; // Переходим к следующему символу
    }

    // Если последняя строка оказалась самой длинной
    if (cur_col > max_col) {
        max_col = cur_col;
    }

    *need_rows = cur_row;
    *need_cols = max_col;
}


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
                     int from_row)
{
    int rel_row = 0;  // Текущая строка, она же счётчик строк
    int rel_col = 0;  // Текущий столбец

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

    for (const char* p = text; *p; ) {
        size_t char_len = utf8_char_length(p);

        // Если текущая строка достигает максимальной ширины
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
        } else {
            // Обычный печатаемый символ
            if (is_not_skipped_row()) { // Если мы не пропускаем
                fwrite(p, 1, char_len, stdout); // Выводим символ (UTF-8)
            }
            rel_col++; // Увеличиваем счётчик длины строки
            p += char_len; // Переходим к следующему символу
        }
    }

    // Заполнение оставшейся части строки до конца, если необходимо
    if (rel_col != 0) {
        fullfiller();
    }
}


static inline int min(int a, int b) {
    return (a < b) ? a : b;
}


int display_message(MessageNode* message, int x, int y, int max_width, int max_height) {
    if (message == NULL || message->message == NULL) return 0;

    int needed_cols, needed_rows, cursor_row, cursor_col;
    calc_display_size(message->message, max_width, 0, &needed_cols, &needed_rows, &cursor_row, &cursor_col);

    int display_start_row = (needed_rows > max_height) ? needed_rows - max_height : 0;
    int actual_rows = min(needed_rows, max_height);

    display_wrapped(message->message, x, y, max_width, actual_rows, display_start_row);

    return actual_rows;
}

void drawHorizontalLine(int cols, int y, char sym) {
    moveCursor(1, y);
    for (int i = 0; i < cols; i++) {
        putchar(sym);
        fflush(stdout);
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
    bool key_printable = (key < KEY_UNKNOWN);
    if (key_printable) {
        enqueueEvent(CMD, cmd_insert, input_buffer, len);
    } else {
        const KeyMap* command = findCommandByKey(key);
        if (command) {
            // DBG ON
            char logMsg[DBG_LOG_MSG_SIZE] = {0};
            snprintf(logMsg, sizeof(logMsg),
                     "keyb: enque cmd: %s\n", command->cmdName);
            pushMessage(&messageList, logMsg);
            // DBG OFF
            enqueueEvent(CMD, command->cmdFunc, command->param, len);
        } else {
            enqueueEvent(DBG, NULL, input_buffer, len);
        }
    }

    if (len == 1) {
        if (input_buffer[0] == '\x04') {
            // Обрабатываем Ctrl-D для выхода
            printf("\n");
            return true; // (terminate := true)
        }
    }
    return false; // (terminate := false)
}

int margin = 8;

volatile sig_atomic_t sig_winch_raised = false;

void handle_winch(int sig) {
    sig_winch_raised = true;
}

void reDraw() {
    int ib_need_cols = 0, ib_need_rows = 0, ib_cursor_row = 0, ib_cursor_col = 0,
        ib_from_row = 0;

    // Вычисляем относительную позицию курсора в inputbuffer-е
    int rel_max_width = win_cols - margin*2;
    calc_display_size(messageList.current->message, rel_max_width,
                      messageList.current->cursor_pos,
                      &ib_need_cols, &ib_need_rows,
                      &ib_cursor_row, &ib_cursor_col);

    // Очищаем экран
    clearScreen();

    // МИНИБУФЕР ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    // Формируем текст для минибуфера с позицией курсора в строке inputBuffer-a
    // относительной позицией в строке и столбце минибуфера
    char mb_text[1024] = {0};
    snprintf(mb_text, 1024,
             "cur_pos=%d\ncur_row=%d\ncur_col=%d\nib_need_rows=%d\nib_from_row=%d",
             messageList.current->cursor_pos,
             ib_cursor_row, ib_cursor_col, ib_need_rows, ib_from_row);

    int mb_need_cols = 0, mb_need_rows = 0, mb_cursor_row = 0, mb_cursor_col = 0;
    int mb_width = win_cols-2;
    calc_display_size(mb_text, mb_width, 0, &mb_need_cols, &mb_need_rows,
                      &mb_cursor_row, &mb_cursor_col);
    // Когда я вывожу что-то в минибуфер я хочу чтобы при большом
    // выводе он показывал только первые 10 строк
    int max_minibuffer_rows = 10;
    if (mb_need_rows > max_minibuffer_rows)  {
        mb_need_rows = max_minibuffer_rows;
    }
    int mb_from_row = 0;
    int mb_up = win_rows + 1 - mb_need_rows + mb_from_row;
    display_wrapped(mb_text, 2, mb_up, mb_width, mb_need_rows, mb_from_row);
    drawHorizontalLine(win_cols, mb_up-1, '=');
    int bottom = mb_up-2;

    // INPUTBUFFER :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    // Область вывода может быть максимум половиной от оставшейся высоты
    int max_ib_rows = bottom / 2;

    // Если содержимое больше чем область вывода
    if (ib_need_rows > max_ib_rows)  {
        // Тогда надо вывести область где курсор, при этом:
        // Если порядковый номер строки, на которой находится курсор,
        // меньше, чем размер области вывода
        if (ib_cursor_row <= max_ib_rows) {
            // ..то надо выводить с первой строки
            ib_from_row = 0;
            // И выводить сколько сможем
            ib_need_rows = max_ib_rows;
        } else {
            // иначе надо, чтобы курсор был на нижней строке
            ib_from_row = ib_cursor_row - max_ib_rows;
            // И выводить сколько сможем
            ib_need_rows = max_ib_rows;
        }
    } else {
        // По-умолчанию будем выводить от начала содержимого
        ib_from_row = 0;
    }
    // Определяем абсолютные координаты верхней строки
    int up = bottom + 1 - ib_need_rows;
    // Выводим
    if (messageList.current) {
        display_wrapped(messageList.current->message, margin, up, rel_max_width, ib_need_rows, ib_from_row);
    }
    drawHorizontalLine(win_cols, up-1, '-');
    // Возвращаем номер строки выше отображения inputbuffer
    bottom =  up - 2;

    // OUTPUT BUFFER

    int outputBufferAvailableLines = bottom - 1;

    int ob_bottom = outputBufferAvailableLines;
    int maxWidth = win_cols - 2 * margin; // максимальная ширина текста

    MessageNode* current = messageList.tail; // начинаем с последнего элемента
    if (current) {
        current = current->prev; // пропускаем последнее сообщение
    }

    while (current && ob_bottom > 0) {
        int needCols = 0, needRows = 0, cursorRow = 0, cursorCol = 0;
        calc_display_size(current->message, maxWidth, 0,
                          &needCols, &needRows, &cursorRow, &cursorCol);

        if (needRows <= ob_bottom) {
            display_wrapped(current->message, margin, ob_bottom - needRows,
                            maxWidth, needRows, 0);
            ob_bottom -= needRows+1; // обновляем начальную точку для следующего сообщения
        } else {
            display_wrapped(current->message, margin, 0,
                            maxWidth, ob_bottom, needRows - ob_bottom);
            ob_bottom = 0; // заполнили доступное пространство
        }
        current = current->prev; // переходим к предыдущему сообщению
    }

    // Flush
    fflush(stdout);

    // Перемещаем курсор в нужную позицию с поправкой на расположение inputbuffer
    moveCursor(ib_cursor_col + margin, bottom + 1 + ib_cursor_row - ib_from_row);
}


/* Network */

int connect_to_server(const char* server_ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Cannot create socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}


/* Main */

#define READ_TIMEOUT 50000 // 50000 микросекунд (50 миллисекунд)
#define SLEEP_TIMEOUT 100000 // 100 микросекунд
volatile bool need_redraw = true;

int main() {
    const char* initial_text = "Что такое буфер ввода?\nБуфер ввода - это временная область хранения, используемая в вычислительной технике для хранения данных, получаемых от устройства ввода, такого как клавиатура или мышь. Он позволяет системе получать и обрабатывать данные в своем собственном темпе, а не зависеть от скорости их поступления.\nКак работает буфер ввода.\nКак вы набираете текст на клавиатуре, например, нажатия клавиш сохраняются в буфере ввода до тех пор, пока компьютер не будет готов их обработать. Буфер хранит нажатия в том порядке, в котором они были получены, что позволяет обрабатывать их последовательно. Когда компьютер готов, он извлекает данные из буфера и выполняет необходимые действия.\nЧто такое буфер ввода?\nОсновное назначение буфера ввода - отделить устройство ввода от вычислительного блока компьютерной системы. Временное хранение входных данных в буфере позволяет пользователю вводить данные в своем собственном темпе, в то время как компьютер обрабатывает их независимо. Это помогает предотвратить потерю данных и обеспечивает плавное взаимодействие между пользователем и системой.\nМожно ли использовать буфер ввода в программировании?\nДа, буферы ввода обычно используются в программировании для обработки пользовательского ввода. При написании кода вы можете создать буфер ввода для хранения пользовательского ввода до тех пор, пока он не понадобится для дальнейшей обработки. Это позволяет более эффективно обрабатывать пользовательское взаимодействие и обеспечивает бесперебойную работу пользователя.";

    // Выделение памяти под строку и копирование начального текста
    char* inputbuffer_text = NULL;
    inputbuffer_text = (char*)malloc(strlen(initial_text) + 1); // +1 terminator
    if (inputbuffer_text == NULL) {
        fprintf(stderr, "Ошибка: не удалось выделить память для inputbuffer_text.\n");
        return 1;
    }
    strcpy(inputbuffer_text, initial_text);


    initMessageList(&messageList);
    pushMessage(&messageList, initial_text);
    pushMessage(&messageList, "Что такое кольцевой буффер?\nКольцевой буфер, или циклический буфер (англ. ring-buffer) — это структура данных, использующая единственный буфер фиксированного размера таким образом, как будто бы после последнего элемента сразу же снова идет первый. Такая структура легко предоставляет возможность буферизации потоков данных.");
    pushMessage(&messageList, "Разрежённый масси́в — абстрактное представление обычного массива, в котором данные представлены не непрерывно, а фрагментарно; большинство его элементов принимает одно и то же значение (значение по умолчанию, обычно 0 или null). Причём хранение большого числа нулей в массиве неэффективно как для хранения, так и для обработки массива.\nВ разрежённом массиве возможен доступ к неопределенным элементам. В этом случае массив вернет некоторое значение по умолчанию.\nПростейшая реализация этого массива выделяет место под весь массив, но когда значений, отличных от значений по умолчанию, мало, такая реализация неэффективна. К этому массиву не применяются функции для работы с обычными массивами в тех случаях, когда о разрежённости известно заранее (например, при блочном хранении данных). ");
    pushMessage(&messageList, "XOR-связный список — структура данных, похожая на обычный двусвязный список, однако в каждом элементе хранится только один составной адрес — результат выполнения операции XOR над адресами предыдущего и следующего элементов списка.\nДля того, чтобы перемещаться по списку, необходимо иметь адреса двух последовательных элементов.\nВыполнение операции XOR над адресом первого элемента и составным адресом, хранящимся во втором элементе, даёт адрес элемента, следующего за этими двумя элементами.\nВыполнение операции XOR над составным адресом, хранящимся в первом элементе, и адресом второго элемента даёт адрес элемента, предшествующего этим двум элементам.");


    // Отключение буферизации для stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    // Включаем сырой режим
    enableRawMode(STDIN_FILENO);

    char input[MAX_BUFFER]={0};
    int  input_size = 0;
    int  log_window_start = 0;

    // Получаем размер терминала в win_cols и win_rows
    upd_win_size();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa, NULL);


    int sockfd = connect_to_server("127.0.0.1", 8888);


    fd_set read_fds;
    struct timeval timeout;
    int maxfd;

    bool terminate = false;

    reDraw();
    while (!terminate) {
        // initialization for select
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (sockfd != -1) {
            FD_SET(sockfd, &read_fds);
            maxfd = (STDIN_FILENO > sockfd ? STDIN_FILENO : sockfd) + 1;
        } else {
            maxfd = STDIN_FILENO + 1;
        }

        // set timeout
        timeout.tv_sec = 0;
        timeout.tv_usec = READ_TIMEOUT;
        // Select
        int select_result;
        do {
            select_result =
                select(maxfd, &read_fds, NULL, NULL, &timeout);
                /* select(STDIN_FILENO + 1, */
                /*        &read_fds, NULL, NULL, &timeout); */
            if (select_result < 0) {
                if (errno == EINTR) {
                    // Обработка прерывания вызова сигналом
                    // изменения размера окна
                    if (sig_winch_raised) {
                        // Обновляем размеры окна
                        upd_win_size();
                        // Устанавливаем флаг необходимости перерисовки
                        need_redraw = true;
                        // Сбрасываем флаг сигнала
                        sig_winch_raised =  false;
                    }
                    // Сейчас мы должны сразу перейти к отображению..
                } else {
                    // Ошибка, которая не является EINTR
                    perror("select failed");
                    exit(EXIT_FAILURE);
                }
            } else if (select_result == 0) {
                // Обработка таймаута
                usleep(SLEEP_TIMEOUT);
            } else { // select_result > 0
                if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                    // KEYBOARD
                    terminate = keyb();
                    if (processEvents(input, &input_size,
                                      &log_window_start, win_rows)) {
                        /* Тут не нужно дополнительных действий, т.к. */
                        /* далее все будет перерисовано */
                    }
                    need_redraw = true;
                } else if (FD_ISSET(sockfd, &read_fds)) {
                    // NETWORK
                    char buffer[1024];
                    int nread = read(sockfd, buffer, sizeof(buffer) - 1);
                    if (nread > 0) {
                        buffer[nread] = '\0';
                        pushMessage(&messageList, buffer);
                        need_redraw = true;
                    }
                }
            }
            // ОТОБРАЖЕНИЕ (если оно небходимо):
            if (need_redraw) {
                reDraw();
                need_redraw = false;
            }
        } while (select_result < 0 && errno == EINTR);
    }

    clearScreen();

    clearMessageList(&messageList);

    pthread_mutex_destroy(&messageList_mutex);
    pthread_mutex_destroy(&eventQueue_mutex);

    // Очищаем память перед завершением программы
    free(inputbuffer_text);

    return 0;
}
