// event.c

#include "event.h"
#include "msg.h"

MsgList msgList = {0};
pthread_mutex_t msgList_mutex = PTHREAD_MUTEX_INITIALIZER;

// Стеки undo и redo
StateStack* undoStack = NULL;
StateStack* redoStack = NULL;

// Очереди
InputEvent* gInputEventQueue = NULL;
pthread_mutex_t gEventQueue_mutex = PTHREAD_MUTEX_INITIALIZER;

InputEvent* gHistoryEventQueue = NULL;
pthread_mutex_t gHistoryQueue_mutex = PTHREAD_MUTEX_INITIALIZER;

void enqueueEvent(InputEvent** eventQueue,
                  pthread_mutex_t* queueMutex,
                  EventType type, CmdFunc cmdFn, char* seq)
{
    pthread_mutex_lock(queueMutex);

    InputEvent* newEvent =
        (InputEvent*)malloc(sizeof(InputEvent));

    if (newEvent == NULL) {
        perror("Failed to allocate memory for a new event");
        pthread_mutex_unlock(queueMutex);
        exit(1); // Выход при ошибке выделения памяти
    }

    *newEvent = (InputEvent){
        .type = type,
        .cmdFn = cmdFn,
        .seq = seq ? strdup(seq) : NULL,
        .next = NULL
    };

    if (*eventQueue == NULL) {
        *eventQueue = newEvent;
    } else {
        InputEvent* lastEvent = *eventQueue;
        while (lastEvent->next) {
            lastEvent = lastEvent->next;
        }
        lastEvent->next = newEvent;
    }

    pthread_mutex_unlock(queueMutex);
}

#define CMD_FUNC_TABLE                          \
    X(cmd_insert)                               \
    X(cmd_backspace)                            \
    X(cmd_backward_char)                        \
    X(cmd_forward_char)                         \
    X(cmd_undo)                                 \
    X(cmd_redo)                                 \
    X(cmd_to_beginning_of_line)                 \
    X(cmd_to_end_of_line)                       \
    X(cmd_backward_word)                        \
    X(cmd_forward_word)                         \
    X(cmd_toggle_cursor)                        \
    X(cmd_set_marker)                           \
    X(cmd_unset_marker)

typedef struct {
    CmdFunc func;
    const char* description;
} CmdFuncEntry;

CmdFuncEntry cmd_func_table[] = {
#define X(func) {func, #func},
    CMD_FUNC_TABLE
#undef X
    {NULL, NULL} // Terminate the array
};

char* descr_cmd_fn(CmdFunc cmd_fn) {
    for (int i = 0; cmd_func_table[i].func != NULL; i++) {
        if (cmd_func_table[i].func == cmd_fn) {
            return (char*)cmd_func_table[i].description;
        }
    }
    return "cmd_notfound";
}


void convertToAsciiCodes(const char *input, char *output,
                         size_t outputSize)
{
    size_t inputLen = strlen(input);
    size_t offset = 0;

    for (size_t i = 0; i < inputLen; i++) {
        int written = snprintf(output + offset, outputSize - offset,
                               "%x", (unsigned char)input[i]);
        if (written < 0 || written >= outputSize - offset) {
            // Output buffer is full or an error occurred
            break;
        }
        offset += written;
        if (i < inputLen - 1) {
            // Add a space between numbers,
            // but not after the last number
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

bool processEvents(InputEvent** eventQueue,
                   pthread_mutex_t* queueMutex,
                   char* input, int* input_size,
                   int* log_window_start, int rows)
{
    pthread_mutex_lock(queueMutex);
    bool updated= false;
    while (*eventQueue) {
        InputEvent* event = *eventQueue;
        *eventQueue = (*eventQueue)->next;

        switch(event->type) {
        case DBG:
            if (event->seq != NULL) {
                char asciiCodes[ASCII_CODES_BUF_SIZE] = {0};
                convertToAsciiCodes(event->seq, asciiCodes,
                                    ASCII_CODES_BUF_SIZE);
                Key key = identify_key(event->seq,
                                       strlen(event->seq));
                char logMsg[DBG_LOG_MSG_SIZE] = {0};
                snprintf(logMsg, sizeof(logMsg),
                         "[DBG]: %s, [%s]\n",
                         key_to_str(key), asciiCodes);
                pushMessage(&msgList, logMsg);
                updated = true;
            }
            break;
        case CMD:
            if (!event->cmdFn) {
                // Вместо cmdFn обнаружили NULL
                char logMsg[DBG_LOG_MSG_SIZE] = {0};
                snprintf(logMsg, sizeof(logMsg),
                         "case CMD: No command found for: %s\n",
                         event->seq);
                pushMessage(&msgList, logMsg);
            } else if ( (event->cmdFn == cmd_undo)
                        || (event->cmdFn == cmd_redo) ) {
                // Итак, это cmd_redo или cmd_undo - для них не нужно
                // ничего делать с возвращенным состоянием
                // (кроме проверки на NULL), они все сделают сами.
                // Нужно только выполнить команду
                if (event->cmdFn(msgList.curr, event)) {
                    // Если возвращен не NULL, то cообщаем,
                    // что данные для отображения обновились
                    updated = true;
                }
            } else {
                // Это не cmd_redo или cmd_undo, а другая команда.
                // Выполняем команду, применяя к msgList.curr
                // Получаем состояние или NULL, если в результате
                // выполнения cmdFn состояние не было изменено
                State* retState = event->cmdFn(msgList.curr, event);

                // Дальше все зависит от полученного состояния
                if (!retState) {
                    // Если retState - NULL, то это значит, что в
                    // результате выполнения cmdFn ничего не
                    // поменялось - значит ничего не делаем
                } else {
                    // Если текущее состояние не NULL, помещаем его
                    // в undo-стек
                    pushState(&undoStack, retState);

                    // Очистим redoStack, чтобы история не ветвилась
                    clearStack(&redoStack);

                    // и сообщаем что данные для отображения
                    // обновились
                    updated = true;
                }
            }
            break;
        default:
            // Обнаружили странный event type
            char logMsg[DBG_LOG_MSG_SIZE] = {0};
            snprintf(logMsg, sizeof(logMsg), "Unk event type\n");
            pushMessage(&msgList, logMsg);
        } // endswitch

        // Пересоздать обработанное событие в очереди выполненных
        // событий, при этом event->seq дублируется, если необходимо,
        // внутри enqueueEvent(), поэтому здесь ниже его можно
        // освободить
        enqueueEvent(&gHistoryEventQueue, &gHistoryQueue_mutex,
                     event->type, event->cmdFn, event->seq);

        // Освободить событие
        if (event->seq != NULL) {
            free(event->seq);
        }
        free(event);
    }
    pthread_mutex_unlock(queueMutex);
    return updated;
}


State* createState(MsgNode* node, InputEvent* event) {
    if (!node) return NULL;
    State* state = malloc(sizeof(State));
    if (!state) return NULL;
    state->cmdFn = event->cmdFn;
    if (event->seq) {
        state->seq = strdup(event->seq);
    } else {
        state->seq = NULL;
    }
    state->cnt = 1;
    return state;
}


// State Stakes */

void freeState(State* state) {
    if (!state) return;
    free(state);
}

void pushState(StateStack** stack, State* state) {
    StateStack* newElement = malloc(sizeof(StateStack));
    if (!newElement) return;
    newElement->state = state;
    newElement->next = *stack;
    *stack = newElement;
}

State* popState(StateStack** stack) {
    if (!*stack) return NULL;
    StateStack* topElement = *stack;
    State* state = topElement->state;
    *stack = topElement->next;
    free(topElement);
    return state;
}

void clearStack(StateStack** stack) {
    while (*stack) {
        State* state = popState(stack);
        freeState(state);
    }
}

extern int sockfd;

void connect_to_server(const char* server_ip, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        pushMessage(&msgList, "connect_to_server_err: Cannot create socket");
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0) {
        pushMessage(&msgList, "connect_to_server_err: Connect failed");
        close(sockfd);
        sockfd = -1;
    }
}

char* int_to_hex(int value) {
    // Максимум 8 шестнадцатеричных цифр + нуль-терминатор
    int size = sizeof(value) * 2 + 1;
    char* hex = (char*)malloc(size);
    if (!hex) {
        // Не удалось выделить память
        return NULL;
    }

    // Указатель на конец строки
    char* p = hex + size - 1;
    *p = '\0'; // Завершаем строку нулём

    // Обрабатываем число, чтобы перевести его
    // в шестнадцатеричный вид
    // Работаем с беззнаковой версией, чтобы поддержать
    // отрицательные числа
    unsigned int num = (unsigned int)value;
    do {
        int digit = num & 0xF; // Получаем последние 4 бита
        // Преобразуем в символ и записываем в строку
        *--p = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        num >>= 4; // Сдвигаем число на 4 бита вправо
    } while (num); // Повторяем, пока число не станет нулём

    // Переместим строку в начало буфера, если есть ведущие нули
    if (p != hex) {
        char* start = hex;
        // Копируем валидные символы
        while (*p) *start++ = *p++;
        *start = '\0'; // Нуль-терминатор в конец
    }

    // Возвращаем результат
    return hex;
}


int hex_to_int(const char* hex) {
    int result = 0; // Инициализация результата
    if (hex == NULL) return 0; // Проверка на NULL

    for (const char* p = hex; *p; p++) {
        char c = *p;
        result <<= 4;

        if (c >= '0' && c <= '9') {
            result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            result += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            result += c - 'A' + 10;
        } else {
            // В случае недопустимого символа, возвращаем 0
            return 0;
        }
    }

    return result; // Возвращаем конечное значение
}


// Общая функция для управления состояниями и командами
State* manageState(MsgNode* node, InputEvent* event,
                   State* (*action)(MsgNode*, InputEvent*),
                   void (*updatePosition)(MsgNode*, InputEvent*)) {
    // Пытаемся получить предыдущее состояние из undo-стека
    State* prevState = popState(&undoStack);
    State* newState = NULL;

    if (prevState && prevState->cmdFn == action) {
        // Если предыдущее состояние подходит, используем его
        newState = prevState;
        newState->cnt++;
    } else {
        // Иначе создаем новое состояние и возвращаем
        // предыдущее на место
        pushState(&undoStack, prevState);
        newState = createState(node, event);
        newState->cnt = 1;
    }

    // Вызов специфической функции для изменения состояния
    if (updatePosition) {
        updatePosition(node, event);
    }

    return newState;
}


State* cmd_connect(MsgNode* msg, InputEvent* event) {
    connect_to_server("127.0.0.1", 8888);

    return NULL;
}

State* cmd_alt_enter(MsgNode* node, InputEvent* event) {
    /* /\* pushMessage(&msgList, "ALT enter function"); *\/ */
    /* if (!msgnode || !node->text) return NULL; */

    /* // Вычисляем byte_offset, используя текущую позицию курсора */
    /* int byte_offset = */
    /*     utf8_byte_offset(node->text, node->cursor_pos); */

    /* // Выделяем новую память для сообщения */
    /* // +2 для новой строки и нуль-терминатора */
    /* char* new_message = malloc(strlen(node->text) + 2); */
    /* if (!new_message) { */
    /*     perror("Failed to allocate memory for new message"); */
    /*     return NULL; */
    /* } */

    /* // Копируем часть сообщения до позиции курсора */
    /* strncpy(new_message, node->text, byte_offset); */
    /* new_message[byte_offset] = '\n';  // Вставляем символ новой строки */

    /* // Копируем оставшуюся часть сообщения */
    /* strcpy(new_message + byte_offset + 1, node->text + byte_offset); */

    /* // Освобождаем старое сообщение и обновляем указатель */
    /* free(node->text); */
    /* node->text = new_message; */

    /* // Перемещаем курсор на следующий символ после '\n' */
    /* node->cursor_pos++; */

    /* // При необходимости обновляем UI или логику отображения */
    /* // ... */

    return NULL;
}

State* cmd_enter(MsgNode* msg, InputEvent* event) {
    /* pushMessage(&msgList, "enter function"); */
    /* if (msgList.curr == NULL) { */
    /*     pushMessage(&msgList, "cmd_enter_err: Нет сообщения для отправки"); */
    /*     return NULL; */
    /* } */
    /* const char* text = node->text; */
    /* if (text == NULL || strlen(text) == 0) { */
    /*     pushMessage(&msgList, "cmd_enter_err: Текущее сообщение пусто"); */
    /*     return NULL; */
    /* } */
    /* if (sockfd <= 0) { */
    /*     pushMessage(&msgList, "cmd_enter_err: Нет соединения"); */
    /*     return NULL; */
    /* } */

    /* ssize_t bytes_sent = send(sockfd, text, strlen(text), 0); */
    /* if (bytes_sent < 0) { */
    /*     pushMessage(&msgList, "cmd_enter_err: Ошибка при отправке сообщения"); */
    /* } else { */
    /*     pushMessage(&msgList, "cmd_enter: отправлено успешно"); */
    /* } */

    return NULL;
}

// [TODO:gmm] Мы можем делать вложенный undo если подменять стеки
// объектом, который тоже содержит стек

void upd_backward_char(MsgNode* node, InputEvent* event) {
    if (node->cursor_pos > 0) {
        node->cursor_pos--;
    }
}

State* cmd_backward_char(MsgNode* node, InputEvent* event) {
    // Если двигать влево уже некуда, то вернемся
    if (node->cursor_pos == 0) {
        return NULL;
    }

    return manageState(node, event, cmd_backward_char,
                       upd_backward_char);
}

void upd_forward_char(MsgNode* node, InputEvent* event) {
    // Вычислим максимальную позицию курсора
    int len = utf8_strlen(node->text);

    // Перемещаем курсор (">=", чтобы позволить курсору
    // стоять на позиции нулевого терминирующего символа
    if (++node->cursor_pos >= len) {
        node->cursor_pos = len;
    }
}

State* cmd_forward_char(MsgNode* node, InputEvent* event) {
    // Если двигать вправо уже некуда, то вернемся
    if (node->cursor_pos >= utf8_strlen(node->text)) {
        return NULL;
    }

    return manageState(node, event, cmd_forward_char,
                       upd_forward_char);
}

void upd_forward_word(MsgNode* node, InputEvent* event) {
    // Получаем текущую позицию курсора
    int current_pos = node->cursor_pos;

    // Получаем длину текста в символах
    int text_length = utf8_strlen(node->text);
    int byte_offset = utf8_byte_offset(node->text, current_pos);

    // Пропускаем пробельные символы, начиная с текущей позиции
    while (current_pos < text_length
           && isspace((unsigned char)node->text[byte_offset])) {
        byte_offset = utf8_next_char(node->text, byte_offset);
        current_pos++;
    }

    // Продолжаем двигаться вперед до следующего пробельного символа
    while (current_pos < text_length
           && !isspace((unsigned char)node->text[byte_offset])) {
        byte_offset = utf8_next_char(node->text, byte_offset);
        current_pos++;
    }

    // Обновляем позицию курсора в узле
    node->cursor_pos = current_pos;
}

// Перемещение курсора вперед на одно слово
State* cmd_forward_word(MsgNode* node, InputEvent* event) {
    // Если курсор уже в конце строки, то вернемся
    if (node->cursor_pos == utf8_strlen(node->text)) {
        return NULL;
    }

    return manageState(node, event, cmd_forward_word,
                       upd_forward_word);
}

void upd_backward_word(MsgNode* node, InputEvent* event) {
    // Получаем текущую позицию курсора
    int current_pos = node->cursor_pos;
    int byte_offset = utf8_byte_offset(node->text, current_pos);

    // Пропускаем пробельные символы, идя назад
    while (current_pos > 0
           && isspace((unsigned char)node->text[byte_offset - 1])) {
        byte_offset = utf8_prev_char(node->text, byte_offset);
        current_pos--;
    }

    // Продолжаем двигаться назад до следующего пробельного
    // символа или начала текста
    while (current_pos > 0
           && !isspace((unsigned char)node->text[byte_offset - 1])) {
        byte_offset = utf8_prev_char(node->text, byte_offset);
        current_pos--;
    }

    // Обновляем позицию курсора в узле
    node->cursor_pos = current_pos;
}

// Перемещение курсора назад на одно слово
State* cmd_backward_word(MsgNode* node, InputEvent* event) {
    // Если курсор уже в начале строки, ничего не делаем
    if (node->cursor_pos == 0) {
        return NULL;
    }

    // Запоминаем старую позицию курсора
    int old_cursor_pos = node->cursor_pos;

    State* newState =
        manageState(node, event, cmd_backward_word,
                    upd_backward_word);

    if (newState->cnt == 1) {
        // Если в результате мы имеем новое состояние
        // а не взяли предыдущее, то записываем в него
        // старую позицию курсора в виде строки
        newState->seq = int_to_hex(old_cursor_pos);
    }

    return newState;
}

void upd_to_beginning_of_line(MsgNode* node, InputEvent* event) {
    // Смещение в байтах от начала строки до курсора
    int byte_offset =
        utf8_byte_offset(node->text, node->cursor_pos);

    // Движемся назад, пока не найдем начало строки или начало текста
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->text, byte_offset);
        if (node->text[prev_offset] == '\n') {
            // Переходим на следующий символ после '\n'
            byte_offset = utf8_next_char(node->text, prev_offset);
            node->cursor_pos = utf8_strlen(node->text);
            return;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }

    // Если достигли начала текста, устанавливаем курсор на позицию 0
    node->cursor_pos = 0;
}

State* cmd_to_beginning_of_line(MsgNode* node, InputEvent* event) {
    // Если двигать влево уже некуда, то вернемся
    if (node->cursor_pos == 0) {
        return NULL;
    }

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newState->seq
    char* seq = int_to_hex(node->cursor_pos);

    State* newState =
        manageState(node, event, cmd_to_beginning_of_line,
                    upd_to_beginning_of_line);

    // Записываем в него старую позицию курсора
    newState->seq = seq;

    return newState;
}

State* cmd_get_back_to_line_pos(MsgNode* node, InputEvent* event) {
    // seq хранит старую позицию курсора
    // в шестнадцатеричном виде, поэтому декодируем
    int old_cursor_pos = hex_to_int(event->seq);

    // Проверка на выход за пределы допустимого
    if ( (old_cursor_pos < 0)
         || (old_cursor_pos > utf8_strlen(node->text)) ) {
        return NULL;
    }

    // Восстанавливаем старую позицию курсора
    node->cursor_pos = old_cursor_pos;

    return NULL;
}

void upd_to_end_of_line(MsgNode* node, InputEvent* event) {
    // Длина строки в байтах
    int len = strlen(node->text);

    // Смещение в байтах от начала строки до курсора
    int byte_offset =
        utf8_byte_offset(node->text, node->cursor_pos);

    // Движемся вперед, пока не найдем конец строки или конец текста
    while (byte_offset < len) {
        if (node->text[byte_offset] == '\n') {
            node->cursor_pos =
                utf8_char_index(node->text, byte_offset);
            return;
        }
        byte_offset = utf8_next_char(node->text, byte_offset);
        node->cursor_pos++;
    }

    // Если достигли конца текста
    node->cursor_pos = utf8_char_index(node->text, len);
}

State* cmd_to_end_of_line(MsgNode* node, InputEvent* event) {
    // Если двигать вправо уже некуда, то вернемся
    if (node->cursor_pos >= utf8_strlen(node->text)) {
        return NULL;
    }

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newState->seq
    char* seq = int_to_hex(node->cursor_pos);

    State* newState =
        manageState(node, event, cmd_to_end_of_line,
                    upd_to_end_of_line);

    // Записываем в него старую позицию курсора
    newState->seq = seq;

    return newState;
}

void upd_insert(MsgNode* node, InputEvent* event) {
    // Байтовое смещение позиции курсора
    int byte_offset =
        utf8_byte_offset(node->text, node->cursor_pos);

    // Длина вставляемой последовательности в байтах
    int insert_len = strlen(event->seq);

    // Рассчитываем новую длину строки после вставки
    int old_len = strlen(node->text);
    int new_len = old_len + insert_len;

    // Попытка реаллоцировать память для нового текста
    // +1 для нуль-терминатора
    char* new_text = realloc(node->text, new_len + 1);
    if (new_text == NULL) {
        perror("Failed to reallocate memory in upd_insert");
        exit(1);
    }

    // Обновляем указатель на message в node
    node->text = new_text;

    // Сдвиг части строки справа от курсора вправо на insert_len
    memmove(node->text + byte_offset + insert_len, // dest
            node->text + byte_offset,              // src
            strlen(node->text) - byte_offset + 1); // size_t

    // Вставка event->seq в позицию курсора
    memcpy(node->text + byte_offset, event->seq, insert_len);

    // Смещение курсора (выполняется в utf8-символах)
    node->cursor_pos += utf8_strlen(event->seq);

}

// Функция для вставки текста в позицию курсора
State* cmd_insert(MsgNode* node, InputEvent* event) {
    State* newState =
        manageState(node, event, cmd_insert,
                    upd_insert);

    if (newState->cnt > 1) {
        // Если мы тут, значит используется старое состояние
        // из стека и теперь мы должны его скорректировать.
        // Сначала, вернем к единице cnt.
        newState->cnt = 1;
        // Потом допишем event->seq к newState->seq:
        // Вычислим длину insert-sequence в байтах
        int insert_seq_len  = strlen(newState->seq);
        // Вычислим длину в байтах того что хотим добавить
        int event_seq_len = strlen(event->seq);
        // Получится общая длина + терминатор
        int new_seq_len = insert_seq_len + event_seq_len + 1;
        // Реаллоцируем seq в new_seq
        char* new_seq =
            realloc(newState->seq, new_seq_len);
        if (new_seq == NULL) {
            perror("Failed to reallocate memory in cmd_insert");
            exit(1);
        }
        // Записываем завершающий нуль в конец строки
        memset(new_seq + new_seq_len - 2, 0, 1);
        // Дописываем к new_seq новый event->seq (в конец)
        strncpy(new_seq + insert_seq_len,
                event->seq, event_seq_len);
        // Записываем реаллоцированный seq в newState
        newState->seq = new_seq;
    }

    return newState;
}

State* cmd_uninsert(MsgNode* node, InputEvent* event) {
    // Вычисляем сколько utf-8 символов надо удалить
    int cnt_del = utf8_strlen(event->seq);

    // Вычисляем сколько они занимают байт
    int ins_len = strlen(event->seq);

    // Находим байтовую позицию символа под курсором
    int byte_offset =
        utf8_byte_offset(node->text, node->cursor_pos);

    // Новая байтовая позиция будет меньше на
    // размер удаляемой строки в байтах
    int new_byte_offset = byte_offset - ins_len;

    // Если новая байтовая позиция меньше чем
    // начало строки - это сбой
    if (new_byte_offset < 0) {
        perror("Error calc undo insert");
        exit(1);
    }

    // Перемещаем правую часть строки на новую
    // байтовую позицию
    strcpy(node->text + new_byte_offset,
           node->text + byte_offset);
    // Реаллоцируем строку [TODO:gmm] segfault
    /* node->message = */
    /*     realloc(node->message, */
    /*             strlen(node->message) */
    /*             - ins_len); */
    //

    // Перемещаем курсор на кол-во удаленных символов
    node->cursor_pos -= cnt_del;

    return NULL;
}

// [TODO:gmm] Функция для удаления символа справа от курсора (DEL)

State* cmd_backspace(MsgNode* node, InputEvent* event) {
    // If cursor is at the start, no need to backspace
    if (node->cursor_pos == 0) {
        return NULL;
    }

    // В случае, если эта функция вызывается из cmd_redo
    // она может удалять более чем один символ. Информация
    // об этом сохраняется в event->seq, поэтому мы можем
    // определить сколько символов надо удалять
    int cnt_del;
    if (event->seq) {
        cnt_del= utf8_strlen(event->seq);
    } else {
        cnt_del = 1;
    }

    // Находим байтовую позицию текущего UTF-8 символа
    int byte_offset =
        utf8_byte_offset(node->text, node->cursor_pos);

    // Вычисляем новую позицию курсора
    int new_cursor_pos = node->cursor_pos - cnt_del;
    if (new_cursor_pos < 0) {
        new_cursor_pos = 0;
    }

    // Находим байтовую позицию предыдущего символа
    int byte_offset_prev =
        utf8_byte_offset(node->text, new_cursor_pos);

    // Находим длину удаляемого символа в байтах
    int del_len = byte_offset - byte_offset_prev;

    // Здесь будет указатель на удаляемое
    char* del = NULL;

    State* newState =
        manageState(node, event, cmd_backspace, NULL);

    if (newState->cnt > 1) {
        // Если мы тут, значит используется старое состояние
        // из стека и теперь мы должны его скорректировать.
        // Сначала, вернем к единице cnt.
        newState->cnt = 1;
        // Будем присоединять удаляемое к предыдущему состоянию.

        // Определяем, сколько байт содержит newState->seq
        int old_len = strlen(newState->seq);

        // Реаллоцируем newState->seq чтобы вместить туда
        // новое удаляемое и нуль-терминатор
        del = realloc(newState->seq, old_len + del_len + 1);

        // Сдвигаем содержимое newState->seq вправо на del_len
        strncpy(newState->seq + del_len, newState->seq, old_len);

        // Дописываем в начало del удаляемое
        strncpy(del, node->text + byte_offset_prev, del_len);

        // Завершаем удаляемое нуль-терминатором
        memset(del + old_len + del_len, 0, 1);

        // Обновляем newState->seq
        newState->seq = del;
    } else {
        // Если мы тут, значит мы используем свежесозданное
        // состояние

        // Выделяем память для удаляемого символа и нуль-терминатора
        del = malloc(del_len + 1);
        if (!del) {
            perror("Failed to allocate memory for deleted");
            return NULL;
        }

        // Копируем удаляемое в эту память
        strncpy(del, node->text + byte_offset_prev, del_len);

        // Завершаем удаляемое нуль-терминатором
        memset(del + del_len, 0, 1);

        // Запоминить в его seq удаляемый символ
        newState->seq = del;

    }

    // :::: Изменение node

    // Вычисляем новую длину text в байтах
    int new_len = strlen(node->text) - del_len + 1;

    // Выделяем память под новый text
    char* new_text = malloc(new_len);
    if (!new_text) {
        perror("Не удалось выделить память");
        return NULL;
    }

    // Копирование части строки до предыдущего символа
    strncpy(new_text, node->text, byte_offset_prev);

    // Копирование части строки после текущего символа
    strcpy(new_text + byte_offset_prev,
           node->text + byte_offset);

    // Освобождаем старый text и присваиваем новый
    free(node->text);
    node->text = new_text;

    // Обновляем позицию курсора
    node->cursor_pos = new_cursor_pos;

    return newState;
}


State* cmd_prev_msg() {
    /* moveToPreviousMessage(&msgList); */

    return NULL;
}

State* cmd_next_msg() {
    /* moveToNextMessage(&msgList); */

    return NULL;
}


/* Clipboard commands  */

typedef struct ClipboardNode {
    char* text;
    struct ClipboardNode* next;
} ClipboardNode;

ClipboardNode* clipboard_head = NULL;

void push_clipboard(const char* text) {
    ClipboardNode* new_node = malloc(sizeof(ClipboardNode));
    if (new_node == NULL) return; // Обработка ошибки выделения памяти
    new_node->text = strdup(text);
    new_node->next = clipboard_head;
    clipboard_head = new_node;
}

char* pop_clipboard() {
    if (clipboard_head == NULL) return NULL;
    ClipboardNode* top_node = clipboard_head;
    clipboard_head = clipboard_head->next;
    char* text = top_node->text;
    free(top_node);
    return text;
}

State* cmd_copy(MsgNode* node, InputEvent* event) {
    /* int start = min(node->cursor_pos, node->marker_pos); */
    /* int end = max(node->cursor_pos, node->marker_pos); */

    /* int start_byte = utf8_byte_offset(node->text, start); */
    /* int end_byte = utf8_byte_offset(node->text, end); */

    /* if (start_byte != end_byte) { */
    /*     char* text_to_copy = strndup(node->text + start_byte, end_byte - start_byte); */
    /*     push_clipboard(text_to_copy); */
    /*     free(text_to_copy); */
    /* } */

    return NULL;
}

State* cmd_cut(MsgNode* node, InputEvent* event) {
    /* cmd_copy(node, param); // Сначала копируем текст */

    /* int start = min(node->cursor_pos, node->marker_pos); */
    /* int end = max(node->cursor_pos, node->marker_pos); */

    /* int start_byte = utf8_byte_offset(node->message, start); */
    /* int end_byte = utf8_byte_offset(node->message, end); */

    /* memmove(node->message + start_byte, node->message + end_byte, */
    /*         strlen(node->message) - end_byte + 1); */
    /* node->cursor_pos = start; */
    /* node->marker_pos = start; */

    return NULL;
}

State* cmd_paste(MsgNode* node, InputEvent* event) {
    /* char* text_to_paste = pop_clipboard(); */
    /* if (text_to_paste) { */
    /*     cmd_insert(node, text_to_paste); */
    /*     free(text_to_paste); */
    /* } */

    return NULL;
}

void upd_set_marker(MsgNode* node, InputEvent* event) {
    // Присваиваем marker_pos значение cursor_pos
    node->marker_pos = node->cursor_pos;
}

State* cmd_set_marker(MsgNode* node, InputEvent* event) {
    // Если маркер уже установлен, то ничего не делаеме
    if (node->marker_pos != -1) {
        return NULL;
    }

    State* newState =
        manageState(node, event, cmd_set_marker,
                    upd_set_marker);

    if (event->seq) {
        // Если в состоянии есть seq это значит нас вызвал
        // cmd_undo. Восстановим позицию маркера
        node->marker_pos = hex_to_int(event->seq);
    }

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newState->seq
    char* seq = int_to_hex(node->cursor_pos);

    // Записываем в состояние позицию маркера
    newState->seq = seq;

    return newState;  // Возвращаем состояние
}

void upd_unset_marker(MsgNode* node, InputEvent* event) {
    // Присваиваем marker_pos значение -1
    node->marker_pos = -1;
}

State* cmd_unset_marker(MsgNode* node, InputEvent* event) {
    // Если маркер не установлен, то ничего не делаем
    if (node->marker_pos == -1) {
        return NULL;
    }

    // Нам потребуется иметь позицию маркера
    // для будующего сохранения
    int old_marker_pos = node->marker_pos;

    State* newState =
        manageState(node, event, cmd_unset_marker,
                    upd_unset_marker);

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newState->seq
    char* seq = int_to_hex(old_marker_pos);

    // Записываем в состояние старую позицию маркера
    newState->seq = seq;

    return newState;  // Возвращаем состояние
}


State* cmd_toggle_cursor(MsgNode* node, InputEvent* event) {
    int temp = node->cursor_pos;
    node->cursor_pos = node->marker_pos;
    node->marker_pos = temp;

    // Окей, мы переместили курсор, теперь займемся State

    // Указатель на последнее состояние в undo-стеке
    // либо мы его создадим, либо возьмем с вершины стека
    State* currState = NULL;

    // Пытаемся получить предыдущее состояние из undo-стека
    State* prevState = popState(&undoStack);
    if (!prevState) {
        // Если не получилось получить предыдущее состояние
        // то формируем новое состояние, которое будет возвращено
        currState = createState(node, event);
    } else if ( (prevState->cmdFn != cmd_toggle_cursor) ) {
        // Если предыдущее состояние не cmd_toggle_cursor,
        // вернем его обратно
        pushState(&undoStack, prevState);

        // и сформируем для возврата новое состояние
        currState = createState(node, event);
    } else {
        // Иначе, если в undo-стеке существует предыдущее
        // состояние, которое тоже cmd_toggle_cursor.
        // Так как мы извлекли предыдущее состояние, то
        // приравняв его к currState  мы можем возвратить
        // его обратно, только увеличив cnt
        currState = prevState;
        currState->cnt++;
    }

    return currState;
}


State* cmd_undo(MsgNode* node, InputEvent* event) {
    if (undoStack == NULL) {
        pushMessage(&msgList, "No more actions to undo");
        return NULL;
    }

    // Pop the last state from the undo stack
    State* prevState = popState(&undoStack);
    if (!prevState) {
        pushMessage(&msgList, "Failed to pop state from undoStack");
        return NULL;
    }

    // Get the complementary command
    CmdFunc undoCmd = get_comp_cmd(prevState->cmdFn);
    if (undoCmd == NULL) {
        char undoMsg[DBG_LOG_MSG_SIZE] = {0};
        int len =
            (prevState->seq) ? (int)utf8_strlen(prevState->seq) : 0;
        snprintf(undoMsg, sizeof(undoMsg),
                 "Undo: No complementary found for %s <%s>(%d) [%d]",
                 descr_cmd_fn(prevState->cmdFn),
                 prevState->seq, len, prevState->cnt);
        pushMessage(&msgList, undoMsg);
        // Push the state back onto undo stack
        pushState(&undoStack, prevState);
        return NULL;
    }

    // Create a new event to apply the undo command
    InputEvent undoEvent = {
        .type = CMD,
        .cmdFn = undoCmd,
        .seq = prevState->seq ? strdup(prevState->seq) : NULL,
        .next = NULL
    };

    // Apply the undo command
    for (int i = 0; i < prevState->cnt; i++) {
        undoCmd(node, &undoEvent);
    }

    // Перемещаем prevState в redoStack
    pushState(&redoStack, prevState);

    // Clean up
    if (undoEvent.seq) free(undoEvent.seq);

    // Return the prevState to indicate the state has changed
    return prevState;
}

State* cmd_redo(MsgNode* node, InputEvent* event) {
    if (redoStack == NULL) {
        pushMessage(&msgList, "No more actions to redo");
        return NULL;
    }

    // Pop the last state from the redo stack
    State* nextState = popState(&redoStack);
    if (!nextState) {
        pushMessage(&msgList, "Failed to pop state from redoStack");
        return NULL;
    }

    // Create a new event to apply the redo command
    InputEvent redoEvent = {
        .type = CMD,
        .cmdFn = nextState->cmdFn,
        .seq = nextState->seq ? strdup(nextState->seq) : NULL,
        .next = NULL
    };

    // Apply the redo command
    for (int i = 0; i < nextState->cnt; i++) {
        nextState->cmdFn(node, &redoEvent);
    }

    // Push the next state back onto the undo stack
    pushState(&undoStack, nextState);

    // Clean up
    if (redoEvent.seq) { free(redoEvent.seq); }

    // Return the nextState to indicate the state has changed
    return nextState;
}


CmdPair comp_cmds[] = {
    { cmd_insert,                cmd_uninsert },
    { cmd_backspace,             cmd_insert },
    { cmd_forward_char,          cmd_backward_char },
    { cmd_backward_char,         cmd_forward_char },
    { cmd_to_beginning_of_line,  cmd_get_back_to_line_pos },
    { cmd_to_end_of_line,        cmd_get_back_to_line_pos },
    { cmd_backward_word,         cmd_get_back_to_line_pos },
    { cmd_forward_word,          cmd_get_back_to_line_pos },
    { cmd_toggle_cursor,         cmd_toggle_cursor },
    { cmd_set_marker,            cmd_unset_marker },
    { cmd_unset_marker,          cmd_set_marker },
    // ...
};

CmdFunc get_comp_cmd (CmdFunc cmd) {
    for (int i = 0; i < (sizeof(comp_cmds) / sizeof(CmdPair)); i++) {
        if (comp_cmds[i].redo_cmd == cmd) {
            return comp_cmds[i].undo_cmd;
        }
    }
    // If no complementary command is found, return NULL
    return NULL;
}

State* cmd_stub(MsgNode* msg, InputEvent* event) {
    return NULL;
}
