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

#define MAX_BUFFER 1024

char miniBuffer[MAX_BUFFER] = {0};

void addLogLine(const char* text);
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
    CTRL_X,
    SPECIAL
} EventType;

typedef struct InputEvent {
    EventType type;
    char c;
    char* sequence;
    struct InputEvent* next;
} InputEvent;

InputEvent* eventQueue = NULL;

void enqueueEvent(EventType type, char c, char* seq) {
    InputEvent* newEvent = malloc(sizeof(InputEvent));
    newEvent->type = type;
    newEvent->c = c;
    newEvent->sequence = seq ? strdup(seq) : NULL;
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

bool processEvents(char* input, int* input_size, int* cursor_pos,
                   int* log_window_start, int rows, int logSize)
{
    bool updated = false;
    while (eventQueue) {
        InputEvent* event = eventQueue;
        EventType type = event->type;
        char c = event->c;
        eventQueue = eventQueue->next;

        switch(type) {
        case SPECIAL:
            if (event->sequence != NULL) {
                char logMessage[1024];  // Достаточный размер
                snprintf(logMessage, sizeof(logMessage),
                         "Special sequence received: %s | %c",
                         event->sequence, event->c);
                addLogLine(logMessage);  // Добавляем в лог
                //
                if (strcmp(event->sequence, "[1p") == 0) { // Ctrl+Alt+p
                    (*log_window_start)--;
                    updated = true;
                } else if (strcmp(event->sequence, "[1n") == 0) { // Ctrl+Alt+n
                    (*log_window_start)++;
                    updated = true;
                }
            } else {
                addLogLine("Special key without sequence received");
            }
            // Освобождаем строку после использования
            if (event->sequence != NULL) {
                free(event->sequence);
            }
            break;
        case CTRL_X:
            addLogLine("Ctrl+X received");
            /* if (isCtrlStackEmpty()) { */
            /*     pushCtrlStack(c); */
            /* } else { */
            /*     /\* Повторный ввод 'C-x' - Ошибка ввода команд, очистим CtrlStack *\/ */
            /*     clearCtrlStack(); */
            /* } */
            break;
        case BACKSPACE:
            printf("\b \b");  // Удаляем символ в терминале
            if (*input_size > 0 && *cursor_pos > 0) {
                memmove(&input[*cursor_pos - 1],
                        &input[*cursor_pos],
                        *input_size - *cursor_pos);
                input[--(*input_size)] = '\0';
                (*cursor_pos)--;
                updated = true;
            }
            break;
        case TEXT_INPUT:
            if (isCtrlStackEmpty()) {
                /* Обычный ввод */
                if (c == '\n') {
                    input[*input_size] = '\0'; // Ensure string is terminated
                    addLogLine(input);
                    memset(input, 0, *input_size);
                    *input_size = 0;
                    *cursor_pos = 0;
                    updated = true;
                } else {
                    if (*input_size < MAX_BUFFER - 1) {
                        memmove(&input[*cursor_pos + 1],
                                &input[*cursor_pos],
                                *input_size - *cursor_pos);
                        input[*cursor_pos] = c;
                        (*input_size)++;
                        (*cursor_pos)++;
                        updated = true;
                    }
                }
            } else {
                /* Ввод команд */
                if (c == 'p') {
                    /* SCROLL_UP */
                    (*log_window_start)--;
                    if (*log_window_start < 0) {
                        *log_window_start = 0;
                    }
                    updated = true;
                } else if (c == 'n') {
                    /* SCROLL_DOWN: */
                    (*log_window_start)++;
                    if (*log_window_start > logSize - (rows - 1)) {
                        *log_window_start = logSize - (rows - 1);
                    }
                    updated = true;
                } else {
                    /* Ошибка ввода команд, выходим из режима ввода команд */
                    clearCtrlStack();
                }
            }
            break;
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

    while (1) {
        // Переинициализация структур для select
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        // Устанавливаем время ожидания
        timeout.tv_sec = 0;
        timeout.tv_usec = READ_TIMEOUT;
        // Вызываем Select
        int select_result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result > 0) {
            if (read(STDIN_FILENO, &nc, 1) > 0) {
                // Обрабатываем комбинацию Ctrl-D для выхода
                if (nc == '\x04') {
                    printf("\n");
                    break;
                } else if (nc == '\033') {
                    // ESC - символ начала управляющей последовательности
                    // TODO: считывать всю последовательность
                    /* enqueueSpecialEvent(); */
                    enqueueEvent(SPECIAL, '\0', NULL);
                    /* // Проверяем не закончилась ли последовательность */
                    /* if ( (c >= 'A' && c <= 'Z') || */
                    /*      (c >= 'a' && c <= 'z') || */
                    /*      (c == '~' ) ) { */
                    /*     break; */
                } else if (nc == '\x18') { // 'C-x'
                    enqueueEvent(CTRL_X, '\0', NULL);
                } else if (nc == 127 || nc == 8) { // backspace
                    enqueueEvent(BACKSPACE, '\0', NULL);
                } else {
                    enqueueEvent(TEXT_INPUT, nc, NULL);
                }
                // Обработка:
                // Обрабатываем события в конце каждой итерации
                if (processEvents(input, &input_size, &cursor_pos,
                          &log_window_start, rows, logSize)) {
                    /* Тут не требуется дополнительных действий, т.к. в начале */
                    /* следующего цикла все будет перерисовано */
                }
                // Отображение:
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
                // ...
                /* write(STDOUT_FILENO, &nc, 1);  // Эмулируем поведение ECHO */
                /* write(STDOUT_FILENO, "-", 1);  // Эмулируем поведение ECHO */
            }
        } else if (select_result == 0) {
            // Обработка таймаута
            usleep(SLEEP_TIMEOUT);
        } else {
            perror("select failed");
            exit(EXIT_FAILURE);
        }
    }
    freeLog();
    return 0;
}
