#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>

#define MAX_BUFFER 1024

void addLogLine(const char* text);
void disableRawMode();
void enableRawMode();
void clearScreen();
void moveCursor(int x, int y);
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
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void clearScreen() {
    printf("\033[H\033[J");
}

void moveCursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

/* MiniBuffer */

char miniBuffer[MAX_BUFFER] = {0};

void updateMiniBuffer(const char* command) {
    snprintf(miniBuffer, sizeof(miniBuffer), "Command: %s", command);
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
    TEXT_INPUT,
    BACKSPACE,
    CTRL_X
} EventType;

typedef struct InputEvent {
    EventType type;
    char c;
    struct InputEvent* next;
} InputEvent;

InputEvent* eventQueue = NULL;

void enqueueEvent(EventType type, char c) {
    InputEvent* newEvent = malloc(sizeof(InputEvent));
    newEvent->type = type;
    newEvent->c = c;
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
        InputEvent* temp = eventQueue;
        EventType type = temp->type;
        char c = temp->c;
        eventQueue = eventQueue->next;

        switch(type) {
        case CTRL_X:
            if (isCtrlStackEmpty()) {
                pushCtrlStack(c);
            } else {
                /* Повторный ввод 'C-x' - Ошибка ввода команд, очистим CtrlStack */
                clearCtrlStack();
            }
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

        free(temp);
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


void printLogWindow(int startLine, int lineCount) {
    LogLine* current = logHead;
    int lineNum = 0;

    while (current && lineNum < startLine) {
        current = current->next;
        lineNum++;
    }

    while (current && lineCount--) {
        printf("%s\n", current->text);
        current = current->next;
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
    int lines = 0;
    int line_pos = 0;
    // Подсчитываем, сколько строк нужно для отображения input
    for (int i = 0; i < strlen(input); ++i) {
        if (input[i] == '\n') {
            lines++;
            line_pos = 0;
        } else {
            line_pos++;
            if (line_pos >= cols) {
                lines++;
                line_pos=0;
            }
        }
    }
    // Выводим разделитель
    drawHorizontalLine(cols, bottom - lines - 1, '-');
    // Выводим input
    printf("\033[%d;%dH", bottom - lines, 1);
    for (int i = 0; i < strlen(input); ++i) {
        if (input[i] == '\n') {
            putchar(input[i]);
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

int main() {
    enableRawMode();

    char input[MAX_BUFFER];
    int input_size = 0;
    int cursor_pos = 0;  // Позиция курсора в строке ввода
    int rows, cols;
    int log_window_start = 0;
    bool followTail = true; // Флаг для отслеживания, показывать ли последние команды

    // Получаем размер терминала
    printf("\033[18t");
    fflush(stdout);
    scanf("\033[8;%d;%dt", &rows, &cols);

    while (1) {
        clearScreen();

        // Отображаем минибуфер и получаем номер строки над ним
        int bottom = showMiniBuffer(miniBuffer, cols, rows);

        // Отображаем InputBuffer и получаем номер строки над ним
        // NB!: Функция сохраняет позицию курсора с помощью escape-последовательности
        bottom = showInputBuffer(input, cursor_pos, cols, bottom);

        int outputBufferAvailableLines = bottom - 1;

        if (followTail) {
            // Определяем, с какой строки начать вывод,
            // чтобы показать только последние команды
            if (logSize - outputBufferAvailableLines > 0) {
                log_window_start = logSize - outputBufferAvailableLines;
            } else {
                log_window_start = 0;
            }
        }

        // Показываем OutputBuffer в оставшемся пространстве
        showOutputBuffer(log_window_start, logSize, cols, outputBufferAvailableLines);

        // Восстанавливаем сохраненную в функции showInputBuffer позицию курсора
        printf("\033[u");

        // Читаем ввод
        int c = getchar();
        if (c == '\x18') { // 'C-x'
            enqueueEvent(CTRL_X, '\0');
        } else if (c == 127 || c == 8) { // backspace
            enqueueEvent(BACKSPACE, '\0');
        } else {
            enqueueEvent(TEXT_INPUT, c);
        }

        // Обрабатываем события в конце каждой итерации
        if (processEvents(input, &input_size, &cursor_pos,
                          &log_window_start, rows, logSize)) {
            /* Тут не требуется дополнительных действий, т.к. в начале */
            /* следующего цикла все будет перерисовано */
        }
    }

    freeLog();
    return 0;
}
