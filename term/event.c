// event.c

#include "event.h"
#include "msg.h"
#include "client.h"


MsgList msgList = {0};
pthread_mutex_t msgList_mutex = PTHREAD_MUTEX_INITIALIZER;

// Стеки undo и redo
ActionStackElt* undoStack = NULL;
ActionStackElt* redoStack = NULL;

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
        exit(EXIT_FAILURE); // Выход при ошибке выделения памяти
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
    X(cmd_unset_marker)                         \
    X(cmd_enter)

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
                   int* log_window_start, int rows) {
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
                // Итак, это cmd_redo или cmd_undo -
                // для них не нужно ничего делать с
                // возвращаемым,  кроме проверки на NULL),
                // они все сделают сами.
                // Нужно только выполнить команду
                if (event->cmdFn(msgList.curr, event)) {
                    // Если возвращен не NULL, то cообщаем,
                    // что данные для отображения обновились
                    updated = true;
                }
            } else {
                // Это не cmd_redo или cmd_undo, а другая команда.
                // Выполняем команду, применяя к msgList.curr
                // Получаем Action или NULL, если в результате
                // выполнения cmdFn ничего не было изменено
                Action* retAction = event->cmdFn(msgList.curr, event);

                // Дальше все зависит от полученного экшена
                if (!retAction) {
                    // Если retAction - NULL, то это значит, что в
                    // результате выполнения cmdFn ничего не
                    // поменялось - значит ничего не делаем
                } else {
                    // Если текущий Action не NULL, помещаем его
                    // в undo-стек
                    pushAction(&undoStack, retAction);

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
        // событий. При этом event->seq будет дублирован с помощью
        // strdup() при добавлении в очередь. Поэтому сразу после
        // добавления можно освободить его, а затем и event.
        enqueueEvent(&gHistoryEventQueue, &gHistoryQueue_mutex,
                     event->type, event->cmdFn, event->seq);
        // Освободить seq
        if (event->seq != NULL) {
            free(event->seq);
        }
        // Освободить event
        free(event);
    }
    pthread_mutex_unlock(queueMutex);
    return updated;
}


Action* createAction(MsgNode* node, InputEvent* event) {
    if (!node) return NULL;
    Action* act = malloc(sizeof(Action));
    if (!act) return NULL;
    act->cmdFn = event->cmdFn;
    act->seq = NULL;
    if (event->seq) {
        act->seq = strdup(event->seq);
        if (!act->seq) {
            perror("Failed to allocate memory in createAction");
            exit(EXIT_FAILURE);
        }
    }
    act->cnt = 1;
    return act;
}


// Action Stakes */

void freeAction(Action* act) {
    if (!act) return;
    free(act);
}

void pushAction(ActionStackElt** stack, Action* act) {
    ActionStackElt* newElt = malloc(sizeof(ActionStackElt));
    if (!newElt) {
        perror("Failed to allocate memory in pushAction");
        exit(EXIT_FAILURE);
    }
    newElt->act = act;
    newElt->next = *stack;
    *stack = newElt;
}

Action* popAction(ActionStackElt** stack) {
    if (!*stack) return NULL;
    ActionStackElt* topElt = *stack;
    Action* act = topElt->act;
    *stack = topElt->next;
    free(topElt);
    return act;
}

void clearStack(ActionStackElt** stack) {
    while (*stack) {
        Action* act = popAction(stack);
        freeAction(act);
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


// Общая функция для управления экшеном и командами
Action* mngAct(MsgNode* node, InputEvent* event,
               Action* (*action)(MsgNode*, InputEvent*),
               void (*updateFn)(MsgNode*, InputEvent*)) {
    // Пытаемся получить предыдущий экшн из undo-стека
    Action* prevAction = popAction(&undoStack);
    Action* newAction = NULL;

    if (prevAction && prevAction->cmdFn == action) {
        // Если предыдущий экшн подходит, используем его
        newAction = prevAction;
        newAction->cnt++;
    } else {
        // Иначе возвращаем предыдущий экшн обратно
        // в стек undo и создаем новый экшн, который
        // будет позже тоже записан в этот стек
        pushAction(&undoStack, prevAction);
        newAction = createAction(node, event);
        newAction->cnt = 1;
    }

    // Вызов специфической функции для изменения ноды
    if (updateFn) {
        updateFn(node, event);
    }

    return newAction;
}


Action* cmd_connect(MsgNode* msg, InputEvent* event) {
    connect_to_server("127.0.0.1", 8888);

    return NULL;
}

Action* cmd_alt_enter(MsgNode* node, InputEvent* event) {
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

extern client_t client;

Action* cmd_enter(MsgNode* msg, InputEvent* event) {
    /* pushMessage(&msgList, "enter function"); */

    size_t len = strlen(msg->text);

    if (len == 0) {
        return NULL;
    }

    // Send the message over the network
    if (client.sockfd != -1) {
        if (client_send(&client, (char*)msg->text, len) != 0) {
            pushMessage(&msgList, "Failed to send message");
            exit(1);
        }
    } else {
        // No connection
        pushMessage(&msgList, "Not connected to server");
    }

    // Add the message to the message list for local display
    pushMessage(&msgList, msg->text);

    // Clear the input buffer
    free(msg->text);
    msg->text = strdup("");
    msg->cursor_pos = 0;
    msg->marker_pos = -1;

    // [NOTE:gmm] Можно ли отменить посылку сообщения?
    return NULL;
}

// [TODO:gmm] Мы можем делать вложенный undo если подменять стеки
// объектом, который тоже содержит стек

void upd_backward_char(MsgNode* node, InputEvent* event) {
    if (node->cursor_pos > 0) {
        node->cursor_pos--;
    }
}

Action* cmd_backward_char(MsgNode* node, InputEvent* event) {
    // Если двигать влево уже некуда, то вернемся
    if (node->cursor_pos == 0) {
        return NULL;
    }

    return mngAct(node, event, cmd_backward_char,
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

Action* cmd_forward_char(MsgNode* node, InputEvent* event) {
    // Если двигать вправо уже некуда, то вернемся
    if (node->cursor_pos >= utf8_strlen(node->text)) {
        return NULL;
    }

    return mngAct(node, event, cmd_forward_char,
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
Action* cmd_forward_word(MsgNode* node, InputEvent* event) {
    // Если курсор уже в конце строки, то вернемся
    if (node->cursor_pos == utf8_strlen(node->text)) {
        return NULL;
    }

    return mngAct(node, event, cmd_forward_word,
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
Action* cmd_backward_word(MsgNode* node, InputEvent* event) {
    // Если курсор уже в начале строки, ничего не делаем
    if (node->cursor_pos == 0) {
        return NULL;
    }

    // Запоминаем старую позицию курсора
    int old_cursor_pos = node->cursor_pos;

    Action* newAction =
        mngAct(node, event, cmd_backward_word,
               upd_backward_word);

    if (newAction->cnt == 1) {
        // Если в результате мы имеем новый экшн
        // а не взяли предыдущий, то записываем в него
        // старую позицию курсора в виде строки
        newAction->seq = int_to_hex(old_cursor_pos);
    }

    return newAction;
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

Action* cmd_to_beginning_of_line(MsgNode* node, InputEvent* event) {
    // Если двигать влево уже некуда, то вернемся
    if (node->cursor_pos == 0) {
        return NULL;
    }

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newAction->seq
    char* seq = int_to_hex(node->cursor_pos);

    Action* newAction =
        mngAct(node, event, cmd_to_beginning_of_line,
               upd_to_beginning_of_line);

    // Записываем в него старую позицию курсора
    newAction->seq = seq;

    return newAction;
}

Action* cmd_get_back_to_line_pos(MsgNode* node, InputEvent* event) {
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

Action* cmd_to_end_of_line(MsgNode* node, InputEvent* event) {
    // Если двигать вправо уже некуда, то вернемся
    if (node->cursor_pos >= utf8_strlen(node->text)) {
        return NULL;
    }

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newAction->seq
    char* seq = int_to_hex(node->cursor_pos);

    Action* newAction =
        mngAct(node, event, cmd_to_end_of_line,
               upd_to_end_of_line);

    // Записываем в него старую позицию курсора
    newAction->seq = seq;

    return newAction;
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
        exit(EXIT_FAILURE);
    }

    // Обновляем указатель на message в node
    node->text = new_text;

    // Сдвиг части строки справа от курсора вправо на insert_len
    memmove(node->text + byte_offset + insert_len, // dest
            node->text + byte_offset,              // src
            strlen(node->text) - byte_offset + 1); // size_t

    // Вставка event->seq в позицию курсора
    memcpy(node->text + byte_offset, event->seq, insert_len);

    // Вычисляем размер смещения
    int cursor_offset = utf8_strlen(event->seq);

    // Смещение курсора (выполняется в utf8-символах)
    node->cursor_pos += cursor_offset;

    // Смещение маркера, если он расположен после курсора
    if (node->marker_pos >= node->cursor_pos) {
        node->marker_pos += cursor_offset;
    }
}

// Функция для вставки текста в позицию курсора
Action* cmd_insert(MsgNode* node, InputEvent* event) {
    Action* newAction =
        mngAct(node, event, cmd_insert,
               upd_insert);

    if (newAction->cnt > 1) {
        // Если мы тут, значит используется старый экшн
        // из стека и теперь мы должны его скорректировать.
        // Сначала, вернем к единице cnt.
        newAction->cnt = 1;
        // Потом допишем event->seq к newAction->seq:
        // Вычислим длину insert-sequence в байтах
        int insert_seq_len  = strlen(newAction->seq);
        // Вычислим длину в байтах того что хотим добавить
        int event_seq_len = strlen(event->seq);
        // Получится общая длина + терминатор
        int new_seq_len = insert_seq_len + event_seq_len + 1;
        // Реаллоцируем seq в new_seq
        char* new_seq =
            realloc(newAction->seq, new_seq_len);
        if (new_seq == NULL) {
            perror("Failed to reallocate memory in cmd_insert");
            exit(EXIT_FAILURE);
        }
        // Записываем завершающий нуль в конец строки
        memset(new_seq + new_seq_len - 1, 0, 1);
        // Дописываем к new_seq новый event->seq (в конец)
        strncpy(new_seq + insert_seq_len,
                event->seq, event_seq_len);
        // Записываем реаллоцированный seq в newAction
        newAction->seq = new_seq;
    }

    return newAction;
}

Action* cmd_uninsert(MsgNode* node, InputEvent* event) {
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
        exit(EXIT_FAILURE);
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

Action* cmd_backspace(MsgNode* node, InputEvent* event) {
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

    // Запомним старые позиции курсора и маркера
    int old_cursor_pos = node->cursor_pos;
    int old_marker_pos = node->marker_pos;

    // Вычисляем новую позицию курсора
    int new_cursor_pos = old_cursor_pos - cnt_del;
    if (new_cursor_pos < 0) {
        new_cursor_pos = 0;
    }

    // Вычисляем новую позицию маркера
    int new_marker_pos = old_marker_pos;
    if (old_marker_pos > new_marker_pos) {
        // Если маркер находится после курсора и
        // удаление происходит перед маркером:
        // Необходимо сдвинуть маркер влево на
        // количество удалённых символов.
        new_marker_pos = old_marker_pos - cnt_del;
    }

    // Находим байтовую позицию предыдущего символа
    int byte_offset_prev =
        utf8_byte_offset(node->text, new_cursor_pos);

    // Находим длину удаляемого символа в байтах
    int del_len = byte_offset - byte_offset_prev;

    // Здесь будет указатель на удаляемое
    char* del = NULL;

    Action* newAction =
        mngAct(node, event, cmd_backspace, NULL);

    if (newAction->cnt > 1) {
        // Если мы тут, значит используется старый экшн
        // из стека и теперь мы должны его скорректировать.
        // Сначала, вернем к единице cnt.
        newAction->cnt = 1;
        // Будем присоединять удаляемое к предыдущему экшену

        // Определяем, сколько байт содержит newAction->seq
        int old_len = strlen(newAction->seq);

        // Реаллоцируем newAction->seq чтобы вместить туда
        // новое удаляемое и нуль-терминатор
        del = realloc(newAction->seq, old_len + del_len + 1);

        // Сдвигаем содержимое newAction->seq вправо на del_len
        strncpy(newAction->seq + del_len, newAction->seq, old_len);

        // Дописываем в начало del удаляемое
        strncpy(del, node->text + byte_offset_prev, del_len);

        // Завершаем удаляемое нуль-терминатором
        memset(del + old_len + del_len, 0, 1);

        // Обновляем newAction->seq
        newAction->seq = del;
    } else {
        // Если мы тут, значит мы используем свежесозданный
        // экшн

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
        newAction->seq = del;
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

    // Обновляем позицию маркера
    node->marker_pos = new_marker_pos;

    return newAction;
}


Action* cmd_prev_msg() {
    /* moveToPreviousMessage(&msgList); */

    return NULL;
}

Action* cmd_next_msg() {
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

Action* cmd_copy(MsgNode* node, InputEvent* event) {
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

Action* cmd_cut(MsgNode* node, InputEvent* event) {
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

Action* cmd_paste(MsgNode* node, InputEvent* event) {
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

Action* cmd_set_marker(MsgNode* node, InputEvent* event) {
    // Если маркер уже установлен, то ничего не делаем
    if (node->marker_pos != -1) {
        return NULL;
    }

    Action* newAction =
        mngAct(node, event, cmd_set_marker,
               upd_set_marker);

    if (event->seq) {
        // Если в event есть seq это значит нас вызвал
        // cmd_undo. Восстановим позицию маркера
        node->marker_pos = hex_to_int(event->seq);
    }

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newAction->seq
    char* seq = int_to_hex(node->cursor_pos);

    // Записываем в экшн позицию маркера
    newAction->seq = seq;

    return newAction;  // Возвращаем экшн
}

void upd_unset_marker(MsgNode* node, InputEvent* event) {
    // Присваиваем marker_pos значение -1
    node->marker_pos = -1;
}

Action* cmd_unset_marker(MsgNode* node, InputEvent* event) {
    // Если маркер не установлен, то ничего не делаем
    if (node->marker_pos == -1) {
        return NULL;
    }

    // Нам потребуется иметь позицию маркера
    // для будующего сохранения
    int old_marker_pos = node->marker_pos;

    Action* newAction =
        mngAct(node, event, cmd_unset_marker,
               upd_unset_marker);

    // Делаем из node->cursor_pos строку,
    // которую позже запишем в newAction->seq
    char* seq = int_to_hex(old_marker_pos);

    // Записываем в экшн старую позицию маркера
    newAction->seq = seq;

    return newAction;  // Возвращаем экшн
}

void upd_toggle_cursor(MsgNode* node, InputEvent* event) {
    int temp = node->cursor_pos;
    node->cursor_pos = node->marker_pos;
    node->marker_pos = temp;
}

Action* cmd_toggle_cursor(MsgNode* node, InputEvent* event) {
    return mngAct(node, event, cmd_toggle_cursor,
                  upd_toggle_cursor);
}


Action* cmd_undo(MsgNode* node, InputEvent* event) {
    if (undoStack == NULL) {
        pushMessage(&msgList, "No more actions to undo");
        return NULL;
    }

    // Pop the last action from the undo stack
    Action* prevAction = popAction(&undoStack);
    if (!prevAction) {
        pushMessage(&msgList, "Failed to pop Action from undoStack");
        return NULL;
    }

    // Get the complementary command
    CmdFunc undoCmd = get_comp_cmd(prevAction->cmdFn);
    if (undoCmd == NULL) {
        char undoMsg[DBG_LOG_MSG_SIZE] = {0};
        int len =
            (prevAction->seq) ? (int)utf8_strlen(prevAction->seq) : 0;
        snprintf(undoMsg, sizeof(undoMsg),
                 "Undo: No complementary found for %s <%s>(%d) [%d]",
                 descr_cmd_fn(prevAction->cmdFn),
                 prevAction->seq, len, prevAction->cnt);
        pushMessage(&msgList, undoMsg);
        // Push the Action back onto undo stack
        pushAction(&undoStack, prevAction);
        return NULL;
    }

    // Create a new event to apply the undo command
    InputEvent undoEvent = {
        .type = CMD,
        .cmdFn = undoCmd,
        .seq = prevAction->seq ? strdup(prevAction->seq) : NULL,
        .next = NULL
    };

    // Apply the undo command
    for (int i = 0; i < prevAction->cnt; i++) {
        undoCmd(node, &undoEvent);
    }

    // Перемещаем prevAction в redoStack
    pushAction(&redoStack, prevAction);

    // Clean up
    if (undoEvent.seq) free(undoEvent.seq);

    // Return the prevAction to indicate the Action has changed
    return prevAction;
}

Action* cmd_redo(MsgNode* node, InputEvent* event) {
    if (redoStack == NULL) {
        pushMessage(&msgList, "No more actions to redo");
        return NULL;
    }

    // Pop the last Action from the redo stack
    Action* nextAction = popAction(&redoStack);
    if (!nextAction) {
        pushMessage(&msgList, "Failed to pop Action from redoStack");
        return NULL;
    }

    // Create a new event to apply the redo command
    InputEvent redoEvent = {
        .type = CMD,
        .cmdFn = nextAction->cmdFn,
        .seq = nextAction->seq ? strdup(nextAction->seq) : NULL,
        .next = NULL
    };

    // Apply the redo command
    for (int i = 0; i < nextAction->cnt; i++) {
        nextAction->cmdFn(node, &redoEvent);
    }

    // Push the next Action back onto the undo stack
    pushAction(&undoStack, nextAction);

    // Clean up
    if (redoEvent.seq) { free(redoEvent.seq); }

    // Return the nextAction to indicate the Action has changed
    return nextAction;
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

Action* cmd_stub(MsgNode* msg, InputEvent* event) {
    return NULL;
}
