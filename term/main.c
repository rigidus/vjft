//main.c

#include "all_libs.h"

#include "term.h"
#include "msg.h"
#include "key.h"
#include "ctrlstk.h" // include key.h and key_map.h
/* // Здесь создается enum Key, содержащий перечисления вида */
/* // KEY_A, KEY_B, ... для всех возможных нажимаемых клавиш */
#include "utf8.h"
#include "iface.h"
#include "kbd.h"
#include "event.h"
#include "mbuf.h"
#include "history.h"

#define MAX_BUFFER 1024

extern ActionStackElt* undoStack;
extern ActionStackElt* redoStack;

void drawHorizontalLine(int cols, int y, char32_t sym);

volatile int win_cols = 0;
volatile int win_rows = 0;

void upd_win_size() {
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0)
    {
        // Освобождаем старые буферы
        resize_buffers(size.ws_row, size.ws_col);
        // Обновляем переменные размеров окна
        win_cols = size.ws_col;
        win_rows = size.ws_row;
    }
}

CtrlStack* ctrlStack = NULL;

typedef struct {
    Key* combo;
    int comboLength;
    char* cmdName;
    CmdFunc cmdFunc;
    char* param;
} KeyMap;

// Макрос для создания комбинации клавиш и инициализации KeyMap
#define KEY_COMMAND(cmdName, cmdFunc, param, ...)       \
    ((KeyMap) {                                         \
        (Key[]){ __VA_ARGS__ },                         \
        (sizeof((Key[]){ __VA_ARGS__ }) / sizeof(Key)), \
        cmdName,                                        \
        cmdFunc,                                        \
        NULL                                            \
    }),                                                 \

// Использование макроса для создания элемента массива
KeyMap keyCommands[] = {
    KEY_COMMAND("CMD_CONNECT", cmd_connect, NULL, KEY_CTRL_X, KEY_CTRL_O)
    KEY_COMMAND("CMD_ENTER", cmd_enter, NULL, KEY_ENTER)
    KEY_COMMAND("CMD_ALT_ENTER", cmd_alt_enter, NULL, KEY_ALT_ENTER)
    KEY_COMMAND("CMD_BACKSPACE", cmd_backspace, NULL, KEY_BACKSPACE)
    KEY_COMMAND("CMD_BACKWARD_CHAR", cmd_backward_char, NULL, KEY_CTRL_B)
    KEY_COMMAND("CMD_FORWARD_CHAR", cmd_forward_char, NULL, KEY_CTRL_F)
    KEY_COMMAND("CMD_FORWARD_WORD", cmd_forward_word, NULL, KEY_ALT_F)
    KEY_COMMAND("CMD_BACKWARD_WORD", cmd_backward_word, NULL, KEY_ALT_B)
    KEY_COMMAND("CMD_PREV_MSG", cmd_prev_msg, NULL, KEY_CTRL_P)
    KEY_COMMAND("CMD_NEXT_MSG", cmd_next_msg, NULL, KEY_CTRL_N)
    KEY_COMMAND("CMD_INSERT_K", cmd_insert, "THE_K", KEY_K)
    KEY_COMMAND("CMD_INSERT_L", cmd_insert, "THE_L", KEY_L)
    KEY_COMMAND("CMD_TO_BEGINNING_OF_LINE", cmd_to_beginning_of_line, NULL, KEY_CTRL_A)
    KEY_COMMAND("CMD_TO_END_OF_LINE", cmd_to_end_of_line, NULL, KEY_CTRL_E)
    KEY_COMMAND("CMD_COPY", cmd_copy, NULL, KEY_ALT_W)
    KEY_COMMAND("CMD_CUT", cmd_cut, NULL, KEY_CTRL_W)
    KEY_COMMAND("CMD_PASTE", cmd_paste, NULL, KEY_CTRL_Y)
    KEY_COMMAND("CMD_TOGGLE_CURSOR", cmd_toggle_cursor, NULL, KEY_CTRL_T)
    KEY_COMMAND("CMD_UNDO", cmd_undo, NULL, KEY_CTRL_BACKSPACE)
    KEY_COMMAND("CMD_REDO", cmd_redo, NULL, KEY_ALT_BACKSPACE)
    KEY_COMMAND("CMD_SET_MARKER", cmd_set_marker, NULL, KEY_CTRL_BACKTICK)
    KEY_COMMAND("CMD_UNSET_MARKER", cmd_unset_marker, NULL,
                KEY_SHIFT_ALT_CTRL_2_IS_ALT_CTRL_ATSIGN)
};


#define key_printable (key < KEY_UNKNOWN)

extern char* key_strings;

bool matchesCombo(Key* combo, int length) {
    CtrlStack* current = ctrlStack;
    for (int i = length - 1; i >= 0; i--) {
        if (!current) {
            /* pushMessage(&msgList, strdup("STACK is NULL")); */
        } else {
            /* char fc_text[MAX_BUFFER] = {0}; */
            /* snprintf(fc_text, MAX_BUFFER, */
            /*          "STACK is NOT_NULL (%d): %s", */
            /*          i, */
            /*          key_to_str(current->key)); */
            /* pushMessage(&msgList, strdup(fc_text)); */
        }
        if (!current || current->key != combo[i]) {
            /* char fc_text[MAX_BUFFER] = {0}; */
            /* snprintf(fc_text, MAX_BUFFER, */
            /*          "NOT_MATCH | key:%s | combo[i=%d]:%s", */
            /*          key_to_str(current->key), */
            /*          i, */
            /*          key_to_str(combo[i])); */
            /* pushMessage(&msgList, strdup(fc_text)); */
            return false; // Не совпадает или стек короче комбинации
        }
        /* pushMessage(&msgList, strdup("MAY_BE")); */
        current = current->next;
    }
    /* pushMessage(&msgList, strdup("ALMOST_MATCH")); */
    // Убедимся, что в стеке нет лишних клавиш
    return current == NULL;
}

const KeyMap* findCommandByKey() {
    for (int i = 0; i < sizeof(keyCommands) / sizeof(KeyMap); i++) {
        /* char fc_text[MAX_BUFFER] = {0}; */
        /* snprintf(fc_text, MAX_BUFFER, */
        /*          "FINDCommandByKey: cmd by cmd = %s", */
        /*          keyCommands[i].cmdName); */
        /* pushMessage(&msgList, strdup(fc_text)); */
        if (matchesCombo(keyCommands[i].combo,
                         keyCommands[i].comboLength)) {
            /* pushMessage(&msgList, "MATch!"); */
            return &keyCommands[i];
        }
    }
    return NULL;
}


MsgNode* copyMsgNodes(MsgNode* msgnode);
void freeMsgNodes(MsgNode* msgnode);


#define MAX_INPUT_BUFFER 40

bool keyb () {
    char input_buffer[MAX_INPUT_BUFFER];
    int len = read_utf8_char_or_esc_seq(
        STDIN_FILENO, input_buffer, sizeof(input_buffer));
    if (len <= 0) {
        // Не было считано никаких данных, но это
        // не повод завершать выполенение программы
        return false; // (terminate := false)
    }
    Key key = identify_key(input_buffer, len);
    if (key_printable && isCtrlStackEmpty()) {
        enqueueEvent(&gInputEventQueue, &gEventQueue_mutex,
                     CMD, cmd_insert, input_buffer);
    } else {
        if (key == KEY_CTRL_G) {
            clearCtrlStack();
        } else {
            pushCtrlStack(key);
            const KeyMap* cmd = findCommandByKey();
            if (cmd && cmd->cmdName) {
                // Command found
                // DBG ON
                /* char fc_text[MAX_BUFFER] = {0}; */
                /* snprintf(fc_text, MAX_BUFFER,"enq CMD found=%s", */
                /*          strdup(cmd->cmdName)); */
                /* pushMessage(&msgList, strdup(fc_text)); */
                // DBG  OFF
                enqueueEvent(&gInputEventQueue,
                             &gEventQueue_mutex,
                             CMD, cmd->cmdFunc, cmd->param);
                // Clear command stack after sending command
                clearCtrlStack();
            } else {
                pushMessage(&msgList, "keyb: cmd not found");
                // Command not found
                enqueueEvent(&gInputEventQueue,
                             &gEventQueue_mutex,
                             DBG, NULL, input_buffer);
            }
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


// Формируем отображение CtrlStack
void dispCtrlStack() {
    char buffer[MAX_BUFFER / 2] = {0};
    // Указатель на конец буфера
    char *ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';  // Завершающий нуль-символ

    CtrlStack* selt = ctrlStack;
    if (selt) {
        while (selt) {
            const char* tmp = key_to_str(selt->key);
            int len = strlen(tmp);
            // Сдвиг указателя назад на длину строки ключа
            ptr -= len;
            memcpy(ptr, tmp, len); // Копирование строки в буфер
            // Добавляем пробел между элементами, если это
            // не первый элемент
            *(--ptr) = ' ';
            // Следующий элемент стека
            selt = selt->next;
        }
        ptr++; // Убираем ведущий пробел
    }
    appendToMiniBuffer(ptr);
}


// Undo/Redo

void displayStack(ActionStackElt* stack) {
    // Начальный размер буфера
    size_t bufferSize = 256;
    char* buffer = malloc(bufferSize);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        return;
    }
    buffer[0] = '\0';  // Инициализация пустой строки

    ActionStackElt* curStack = stack;
    while (curStack != NULL) {
        Action* act = curStack->act;
        if (act) {
            // Достаточно большой для одного описания
            char descr[128] = {0};
            snprintf(descr, sizeof(descr),
                     "(%s «%s» %d) ",
                     descr_cmd_fn(act->cmdFn),
                     act->seq ? act->seq : "null",
                     act->cnt);

            size_t neededSize =
                strlen(buffer) + strlen(descr) + 1;

            if (neededSize > bufferSize) {
                bufferSize *= 2;  // Удвоение размера при необходимости
                char* newBuffer = realloc(buffer, bufferSize);
                if (!newBuffer) {
                    perror("Failed to realloc for buffer");
                    free(buffer);
                    return;
                }
                buffer = newBuffer;
            }
            strcat(buffer, descr);
        }
        curStack = curStack->next;
    }

    appendToMiniBuffer(buffer);

    free(buffer);
}


int margin = 8;

volatile sig_atomic_t sig_winch_raised = false;

void handle_winch(int sig) {
    sig_winch_raised = true;
}

void reDraw() {
    int ib_need_cols = 0, ib_need_rows = 0,
        ib_cursor_row = 0, ib_cursor_col = 0,
        ib_from_row = 0,
        ib_abs_x = 0, ib_abs_y = 0;

    // Вычисляем относительную позицию курсора в inputbuffer-е
    int ib_rel_max_width = 20;
    calc_display_size(msgList.curr->text, ib_rel_max_width,
                      msgList.curr->cursor_pos,
                      &ib_need_cols, &ib_need_rows,
                      &ib_cursor_row, &ib_cursor_col);

    // МИНИБУФЕР ::::::::::::::::::::::::::::::::::::::::::::

    clearMiniBuffer();

    // Формируем текст для минибуфера с позицией курсора
    // в строке inputBuffer-a относительной позицией в
    // строке и столбце минибуфера
    char mb_text[MAX_BUFFER] = {0};
    snprintf(mb_text, MAX_BUFFER,
             "cur_pos=%d\nmar_pos=%d\ncur_row=%d\n"
             "cur_col=%d\nib_need_rows=%d\nib_from_row=%d\n",
             msgList.curr->cursor_pos, msgList.curr->marker_pos,
             ib_cursor_row, ib_cursor_col, ib_need_rows,
             ib_from_row);
    appendToMiniBuffer(mb_text);

    dispCtrlStack();

    /* dispExEv(gHistoryEventQueue); */

    appendToMiniBuffer("\nUndoStack: ");
    displayStack(undoStack);
    appendToMiniBuffer("\nRedoStack: ");
    displayStack(redoStack);

    // Отображение минибуфера
    /* int mb_max_col = win_cols-2, mb_max_row = 10; */
    /* int max_lines = miniBuffer.size + 1; */
    /* LineInfo* lines = malloc(max_lines * sizeof(LineInfo)); */
    /* max_lines = get_lines(miniBuffer.buffer, */
    /*                        mb_max_col, mb_max_row, lines); */

    /* int mb_scroll_offset = 0; */
    /* render_text_window(miniBuffer.buffer, */
    /*                    1, win_rows - max_lines, */
    /*                    mb_max_col, mb_max_row, */
    /*                    -1, -1, */
    /*                    &mb_scroll_offset); */
    int max_lines = 10;
    display_wrapped(
        miniBuffer.buffer,
        1, win_rows - max_lines,
        win_cols-2, win_rows,
        0,
        -1,
        -1);


    int bottom = win_rows - max_lines - 2;

    // INPUTBUFFER ::::::::::::::::::::::::::::::::::::::::::

    // Область вывода может быть максимум половиной
    // от оставшейся высоты
    int max_ib_rows = bottom / 2;

    // Если содержимое больше чем область вывода
    if (ib_need_rows > max_ib_rows)  {
        // Тогда надо вывести область где курсор, при этом:
        // Если порядковый номер строки, на которой находится
        // курсор, меньше, чем размер области вывода
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
    if (msgList.curr) {
        display_wrapped(
            msgList.curr->text,
            ib_abs_x, ib_abs_y,
            ib_rel_max_width,
            ib_need_rows,
            ib_from_row,
            msgList.curr->cursor_pos,
            msgList.curr->marker_pos);
    }
    drawHorizontalLine(win_cols, up-1, U'―');
    // Возвращаем номер строки выше отображения inputbuffer
    bottom =  up - 2;

    // OUTPUT BUFFER

    int outputBufferAvailableLines = bottom - 1;

    int ob_bottom = outputBufferAvailableLines;
    // максимальная ширина текста
    int maxWidth = win_cols - 2 * margin;
    // начинаем с последнего элемента
    MsgNode* current = msgList.tail;
    if (current) {
        // пропускаем последнее сообщение
        current = current->prev;
    }

    while (current && ob_bottom > 0) {
        int needCols = 0, needRows = 0,
            cursorRow = 0, cursorCol = 0;

        calc_display_size(current->text, maxWidth, 0,
                          &needCols, &needRows,
                          &cursorRow, &cursorCol);

        if (needRows <= ob_bottom) {
            display_wrapped(current->text, margin,
                            ob_bottom - needRows,
                            maxWidth, needRows, 0, -1, -1);
            // обновляем начальную точку для
            // следующего сообщения
            ob_bottom -= needRows+1;
        } else {
            display_wrapped(current->text, margin, 0,
                            maxWidth, ob_bottom,
                            needRows - ob_bottom, -1, -1);
            ob_bottom = 0; // заполнили доступное пространство
        }
        // переходим к предыдущему сообщению
        current = current->prev;
    }

    // Flush
    fflush(stdout);

    // После завершения рендеринга
    render_screen();
    clear_screen_buffer(back_buffer);

    /* // Перемещаем курсор в нужную позицию с поправкой */
    /* // на расположение inputbuffer */
    /* moveCursor(ib_cursor_col + ib_abs_x, */
    /*            ib_cursor_row + ib_abs_y - ib_from_row -1); */
}

// Функции для копирования и очистки узлов списка сообщений
MsgNode* copyMsgNodes(MsgNode* msgnode) {
    if (!msgnode) return NULL;

    MsgNode* newNode = malloc(sizeof(MsgNode));
    if (!newNode) return NULL;
    newNode->text = strdup(msgnode->text);
    newNode->cursor_pos = msgnode->cursor_pos;
    newNode->marker_pos = msgnode->marker_pos;
    newNode->next = copyMsgNodes(msgnode->next);

    if (newNode->next) {
        newNode->next->prev = newNode;
    }

    return newNode;
}

void freeMsgNodes(MsgNode* msgnode) {
    if (!msgnode) return;
    freeMsgNodes(msgnode->next);
    free(msgnode->text);
    free(msgnode);
}


// Отображение истории состяний

Action* peekAction(const ActionStackElt* stack) {
    if (!stack) {
        return NULL;  // Возвращаем NULL, если стек пуст
    }
    // Возвращаем экшн на вершине стека без его удаления
    return stack->act;
}

// Инициализация состояния программы

void reinitializeState() {
    initMsgList(&msgList);
    pushMessage(&msgList, "");
}


// Main

#define READ_TIMEOUT 50000 // 50000 микросекунд (50 миллисекунд)
#define SLEEP_TIMEOUT 100000 // 100 микросекунд
volatile bool need_redraw = true;

int sockfd = -1;

int main() {
    reinitializeState();

    // Отключение буферизации для stdout
    setvbuf(stdout, NULL, _IONBF, 0);

    // Включаем сырой режим
    enableRawMode(STDIN_FILENO);

    char input[MAX_BUFFER]={0};
    int  input_size = 0;
    int  log_window_start = 0;

    // Получаем размер терминала в win_cols и win_rows
    upd_win_size();

    // Инициализируем буферы
    init_buffers(win_rows, win_cols);

    // Устанавливаем обработчик сигнала
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa, NULL);

    initMiniBuffer(128);


    /* connect_to_server("127.0.0.1", 8888); */


    fd_set read_fds;
    struct timeval timeout;
    int maxfd;

    bool terminate = false;

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
                        // Устанавливаем флаг перерисовки
                        need_redraw = true;
                        // Сбрасываем флаг сигнала
                        sig_winch_raised =  false;
                    }
                    // Сейчас мы должны сразу перейти
                    // к отображению..
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
                    if (processEvents(&gInputEventQueue,
                                      &gEventQueue_mutex,
                                      input, &input_size,
                                      &log_window_start, win_rows)) {
                        /* Тут не нужно дополнительных действий, т.к. */
                        /* далее все будет перерисовано */
                    }
                    need_redraw = true;
                } else if (FD_ISSET(sockfd, &read_fds)) {
                    // NETWORK
                    char buffer[MAX_BUFFER];
                    int nread = read(sockfd, buffer, sizeof(buffer) - 1);
                    if (nread > 0) {
                        buffer[nread] = '\0';
                        pushMessage(&msgList, buffer);
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

    clearMsgList(&msgList);

    pthread_mutex_destroy(&msgList_mutex);
    pthread_mutex_destroy(&gEventQueue_mutex);

    clearStack(&redoStack);
    clearStack(&undoStack);

    freeMiniBuffer();

    return 0;
}
