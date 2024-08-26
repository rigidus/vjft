//main.c

#include "all_libs.h"

#include "term.h"
#include "msg.h"
#include "key.h"
#include "ctrlstk.h" // include key.h and key_map.h
/* // Здесь создается enum Key, содержащий перечисления вида */
/* // KEY_A, KEY_B, ... для всех возможных нажимаемых клавиш */
#include "cmd.h"
#include "utf8.h"
#include "iface.h"

#define MAX_BUFFER 1024

char miniBuffer[MAX_BUFFER] = {0};

void drawHorizontalLine(int cols, int y, char sym);

/* %%term%% */

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

/* %%msg%% */

MessageList messageList = {0};
pthread_mutex_t messageList_mutex = PTHREAD_MUTEX_INITIALIZER;


/* InputEvent */

typedef enum {
    DBG,
    CMD
} EventType;


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

/* %%ctrlstack%% */

CtrlStack* ctrlStack = NULL;

/* %%utf8 */

/* %%cmd%% */

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
    // Другие команды...
    /* {{KEY_CTRL_P}, "CMD_PREV_MSG", cmd_prev_msg, NULL}, */
    /* {KEY_CTRL_N, "CMD_NEXT_MSG", cmd_next_msg, NULL}, */
    /* {KEY_CTRL_B, "CMD_BACKWARD_CHAR", cmd_backward_char, NULL}, */
    /* {KEY_CTRL_F, "CMD_FORWARD_CHAR", cmd_forward_char, NULL}, */
    /* {KEY_ALT_F, "CMD_FORWARD_WORD", cmd_forward_word, NULL}, */
    /* {KEY_ALT_B, "CMD_BACKWARD_WORD", cmd_backward_word, NULL}, */
    /* /\* {KEY_CTRL_A, "CMD_MOVE_TO_BEGINNING_OF_LINE", */
    /*    cmd_move_to_beginning_of_line, NULL}, *\/ */
    /* {KEY_CTRL_E, "CMD_MOVE_TO_END_OF_LINE", cmd_move_to_end_of_line, NULL}, */
    /* {KEY_K, "CMD_INSERT_K", cmd_insert, "=ey"}, */
    /* {KEY_L, "CMD_INSERT_L", cmd_insert, "-ol"}, */
    /* {KEY_CTRL_X, "CMD_CTRL_X", NULL, NULL}, */
    /* {KEY_CTRL_O, "CMD_CONNECT", cmd_connect, NULL}, */
    /* {KEY_CTRL_G, "CMD_CANCEL", cmd_cancel, NULL}, */
};


#define key_printable (key < KEY_UNKNOWN)

extern char* key_strings;

bool matchesCombo(Key* combo, int length) {
    CtrlStack* current = ctrlStack;
    for (int i = length - 1; i >= 0; i--) {
        if (!current) {
            /* pushMessage(&messageList, strdup("STACK is NULL")); */
        } else {
            /* char fc_text[MAX_BUFFER] = {0}; */
            /* snprintf(fc_text, MAX_BUFFER,"STACK is NOT_NULL (%d): %s", */
            /*          i, */
            /*          key_to_str(current->key)); */
            /* pushMessage(&messageList, strdup(fc_text)); */
        }
        if (!current || current->key != combo[i]) {
            /* char fc_text[MAX_BUFFER] = {0}; */
            /* snprintf(fc_text, MAX_BUFFER,"NOT_MATCH | key:%s | combo[i=%d]:%s", */
            /*          key_to_str(current->key), */
            /*          i, */
            /*          key_to_str(combo[i])); */
            /* pushMessage(&messageList, strdup(fc_text)); */
            return false; // Не совпадает или стек короче комбинации
        }
        /* pushMessage(&messageList, strdup("MAY_BE")); */
        current = current->next;
    }
    /* pushMessage(&messageList, strdup("ALMOST_MATCH")); */
    return current == NULL; // Убедимся, что в стеке нет лишних клавиш
}

const KeyMap* findCommandByKey() {
    for (int i = 0; i < sizeof(keyCommands) / sizeof(KeyMap); i++) {

        char fc_text[MAX_BUFFER] = {0};
        snprintf(fc_text, MAX_BUFFER,"FINDCommandByKey: cmd by cmd = %s",
                 keyCommands[i].cmdName);
        pushMessage(&messageList, strdup(fc_text));

        if (matchesCombo(keyCommands[i].combo, keyCommands[i].comboLength)) {
            pushMessage(&messageList, "MATch!");
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


/* %%iface%% */


/* keyboard */

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
    int len = 0, nread = 0;
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
            // Считано два символа и это CSI или SS3 последовательности
            // - читаем дальше
            if (len == 2 && (buf[1] == '[' || buf[1] == 'O')) { continue; }
            // Считано два символа, и второй из них - это \xd0 или \xd1
            // - читаем дальше
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
    if (len <= 0) {
        // Не было считано никаких данных, но это
        // не повод завершать выполенение программы
        return false; // (terminate := false)
    }
    Key key = identify_key(input_buffer, len);
    if (key_printable && isCtrlStackEmpty()) {
        enqueueEvent(CMD, cmd_insert, input_buffer, len);
    } else {
        if (key == KEY_CTRL_G) {
            clearCtrlStack();
        } else {
            pushCtrlStack(key);
            const KeyMap* cmd = findCommandByKey();
            if (cmd && cmd->cmdName) {
                // Command found
                // DBG ON
                char fc_text[MAX_BUFFER] = {0};
                snprintf(fc_text, MAX_BUFFER,"CMD found=%s",strdup(cmd->cmdName));
                pushMessage(&messageList, strdup(fc_text));
                // DBG  OFF
                /*     } else if ( (strcmp(cmd->cmdName, "CMD_CONNECT") == 0) */
                /*                 && (isCtrlStackActive(KEY_CTRL_X)) ) { */
                /*         // Если C-x уже был нажат */
                /*         enqueueEvent(CMD, cmd->cmdFunc, "KEY_CTRL_X", 0); */
                /*         pushMessage(&messageList, "pushed C-o"); */
                /*         clearCtrlStack(); */
                /*     } else if ( (strcmp(cmd->cmdName, "CMD_CANCEL") == 0) */
                /*                 && (isCtrlStackActive(KEY_CTRL_X)) ) { */
                /*         // Если C-x уже был нажат и следующий C-g */
                /*         enqueueEvent(CMD, cmd->cmdFunc, cmd->param, 0); */
                /*         clearCtrlStack(); */
                /*         pushMessage(&messageList, "C-g - clear stack"); */
                /*     } else { */
                /*         // DBG ON */
                /*         char logMsg[DBG_LOG_MSG_SIZE] = {0}; */
                /*         snprintf(logMsg, sizeof(logMsg), */
                /*                  "keyb: enque cmd: %s\n", cmd->cmdName); */
                /*         pushMessage(&messageList, logMsg); */
                /*         // DBG OFF */
                /*         enqueueEvent(CMD, cmd->cmdFunc, cmd->param, len); */

                // Clear command stack after sending command
                clearCtrlStack();
            } else {
                // Command not found
                enqueueEvent(DBG, NULL, input_buffer, len);
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

    // Формируем отображение CtrlStack
    char buffer[MAX_BUFFER / 2] = {0};
    char *ptr = buffer + sizeof(buffer) - 1;  // Указатель на конец буфера
    *ptr = '\0';  // Завершающий нуль-символ

    CtrlStack* selt = ctrlStack;
    if (selt) {
        while (selt) {
            const char* tmp = key_to_str(selt->key);
            int len = strlen(tmp);
            ptr -= len; // Сдвиг указателя назад на длину строки ключа
            memcpy(ptr, tmp, len); // Копирование строки в буфер
            // Добавляем пробел между элементами, если это не первый элемент
            *(--ptr) = ' ';
            // Следующий элемент стека
            selt = selt->next;
        }
        ptr++; // Убираем ведущий пробел
    }

    // Формируем текст для минибуфера с позицией курсора в строке inputBuffer-a
    // относительной позицией в строке и столбце минибуфера
    char mb_text[MAX_BUFFER] = {0};
    snprintf(mb_text, MAX_BUFFER,
             "cur_pos=%d\ncur_row=%d\ncur_col=%d\nib_need_rows=%d\nib_from_row=%d\n%s",
             messageList.current->cursor_pos,
             ib_cursor_row, ib_cursor_col, ib_need_rows, ib_from_row,
             ptr);

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

int sockfd = -1;

void connect_to_server(const char* server_ip, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Cannot create socket");
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect failed");
        close(sockfd);
        sockfd = -1;
    }
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


    connect_to_server("127.0.0.1", 8888);


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
                    char buffer[MAX_BUFFER];
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
