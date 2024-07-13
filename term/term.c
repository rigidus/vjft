#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>

#define MAX_BUFFER 1024

void addLogLine(const char* text);


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


/* InputEvent */

typedef enum {
    TEXT_INPUT,
    ARROW_UP,
    ARROW_DOWN
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

bool processEvents(char* input, int* input_size, int* cursor_pos, int* current_row,
                   int* log_window_start, int rows, int logSize)
{
    bool updated = false;
    while (eventQueue) {
        InputEvent* temp = eventQueue;
        EventType type = temp->type;
        char c = temp->c;
        eventQueue = eventQueue->next;

        if (type == TEXT_INPUT) {
            if (c == '\n') {
                input[*input_size] = '\0'; // Ensure string is terminated
                addLogLine(input);
                memset(input, 0, *input_size);
                *input_size = 0;
                *cursor_pos = 0;
                updated = true;
            } else if (c == 127 || c == 8) { /* backspace */
                if (*input_size > 0 && *cursor_pos > 0) {
                    memmove(&input[*cursor_pos - 1],
                            &input[*cursor_pos],
                            *input_size - *cursor_pos);
                    input[--(*input_size)] = '\0';
                    (*cursor_pos)--;
                    updated = true;
                }
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
        } else if (type == ARROW_UP) {
            (*log_window_start)--;
            if (*log_window_start < 0) {
                *log_window_start = 0;
            }
            updated = true;
        } else if (type == ARROW_DOWN) {
            (*log_window_start)++;
            if (*log_window_start > logSize - (rows - 1)) {
                *log_window_start = logSize - (rows - 1);
            }
            updated = true;
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

void drawHorizontalLine(int cols, int y) {
    moveCursor(1, y);
    for (int i = 0; i < cols; i++) {
        putchar('-');
    }
}


/* Main */

int main() {
    enableRawMode();

    char input[MAX_BUFFER];
    int input_size = 0;
    int cursor_pos = 0;  // Позиция курсора в строке ввода
    int rows, cols;

    // Получаем размер терминала
    printf("\033[18t");
    fflush(stdout);
    scanf("\033[8;%d;%dt", &rows, &cols);

    int current_row = rows;
    int log_window_start = 0;

    while (1) {
        clearScreen();
        printLogWindow(log_window_start, rows - 1);
        drawHorizontalLine(cols, current_row - 1);  // Рисуем линию разделения

        moveCursor(1, current_row); // Курсор в начало окна ввода
        printf("%s", input);

        moveCursor(cursor_pos + 1, current_row);  // Курсор в конец ввода

        int c = getchar();

        if (c == '\033') {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) &&
                read(STDIN_FILENO, &seq[1], 1)) {
                if (seq[0] == '[') {
                    if (seq[1] == 'A') {
                        printf("=&^A=");
                        enqueueEvent(ARROW_UP, '\0');
                    } else if (seq[1] == 'B') {
                        printf("=&^B=");
                        enqueueEvent(ARROW_DOWN, '\0');
                    }
                }
            }
        } else {
            enqueueEvent(TEXT_INPUT, c);
        }

        // Обрабатываем события в конце каждой итерации
        if (processEvents(input, &input_size, &cursor_pos, &current_row,
                          &log_window_start, rows, logSize)) {
            /* Тут не требуется дополнительных действий, т.к. в начале */
            /* следующего цикла все будет перерисовано */
        }
    }

    freeLog();
    return 0;
}
