// event.c

#include "event.h"

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

void enqueueEvent(InputEvent** eventQueue, pthread_mutex_t* queueMutex,
                  EventType type, CmdFunc cmdFn, char* seq)
{
    pthread_mutex_lock(queueMutex);

    InputEvent* newEvent = (InputEvent*)malloc(sizeof(InputEvent));
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

char* descr_cmd_fn(CmdFunc cmd_fn) {
    if (cmd_fn == cmd_insert) return "cmd_insert";
    if (cmd_fn == cmd_backspace) return "cmd_backspace";
    if (cmd_fn == cmd_backward_char) return "cmd_backward_char";
    if (cmd_fn == cmd_forward_char) return "cmd_forward_char";
    return  "cmd_notfound";
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

bool processEvents(InputEvent** eventQueue, pthread_mutex_t* queueMutex,
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
                Key key = identify_key(event->seq, strlen(event->seq));
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
                    // результате выполнения cmdFn ничего не поменялось
                    // - значит ничего не делаем
                } else {
                    // Если текущее состояние не NULL, помещаем его
                    // в undo-стек
                    pushState(&undoStack, retState);

                    // Очистим redoStack, чтобы история не ветвилась
                    clearStack(&redoStack);

                    // и сообщаем что данные для отображения обновились
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


State* createState(MsgNode* msgnode, InputEvent* event) {
    if (!msgnode) return NULL;
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


/* // State Stakes */

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



State* cmd_connect(MsgNode* msg, InputEvent* event) {
    connect_to_server("127.0.0.1", 8888);

    return NULL;
}

State* cmd_alt_enter(MsgNode* msg, InputEvent* event) {
    /* pushMessage(&msgList, "ALT enter function"); */
    if (!msg || !msg->message) return NULL;

    // Вычисляем byte_offset, используя текущую позицию курсора
    int byte_offset = utf8_byte_offset(msg->message, msg->cursor_pos);

    // Выделяем новую память для сообщения
    // +2 для новой строки и нуль-терминатора
    char* new_message = malloc(strlen(msg->message) + 2);
    if (!new_message) {
        perror("Failed to allocate memory for new message");
        return NULL;
    }

    // Копируем часть сообщения до позиции курсора
    strncpy(new_message, msg->message, byte_offset);
    new_message[byte_offset] = '\n';  // Вставляем символ новой строки

    // Копируем оставшуюся часть сообщения
    strcpy(new_message + byte_offset + 1, msg->message + byte_offset);

    // Освобождаем старое сообщение и обновляем указатель
    free(msg->message);
    msg->message = new_message;

    // Перемещаем курсор на следующий символ после '\n'
    msg->cursor_pos++;

    // При необходимости обновляем UI или логику отображения
    // ...

    return NULL;
}

State* cmd_enter(MsgNode* msg, InputEvent* event) {
    pushMessage(&msgList, "enter function");
    if (msgList.curr == NULL) {
        pushMessage(&msgList, "cmd_enter_err: Нет сообщения для отправки");
        return NULL;
    }
    const char* text = msg->message;
    if (text == NULL || strlen(text) == 0) {
        pushMessage(&msgList, "cmd_enter_err: Текущее сообщение пусто");
        return NULL;
    }
    if (sockfd <= 0) {
        pushMessage(&msgList, "cmd_enter_err: Нет соединения");
        return NULL;
    }

    ssize_t bytes_sent = send(sockfd, text, strlen(text), 0);
    if (bytes_sent < 0) {
        pushMessage(&msgList, "cmd_enter_err: Ошибка при отправке сообщения");
    } else {
        pushMessage(&msgList, "cmd_enter: отправлено успешно");
    }

    return NULL;
}

// [TODO:gmm] Мы можем делать вложенный undo если подменять
// стеки объектом, который тоже содержит стек

// [TODO:gmm] Функция для удаления символа справа от курсора

// Функция для удаления символа слева от курсора
State* cmd_backspace(MsgNode* msgnode, InputEvent* event) {
    if ( (!msgnode) || (!msgnode->message)
         || (msgnode->cursor_pos == 0) ) {
        // Если с msgnode что-то не так,
        // отпустим lock и вернемся
        return NULL;
    }

    // Находим байтовую позицию текущего и предыдущего UTF-8 символов
    int byte_offset =
        utf8_byte_offset(msgnode->message, msgnode->cursor_pos);

    // Так как ранее мы проверили, что msgnode->cursor_pos не ноль,
    // не может быть ситуации антипереполнения, и проверять не нужно
    int new_cursor_pos = msgnode->cursor_pos - 1;

    // Находим байтовую позицию предыдущего символа
    int byte_offset_prev =
        utf8_byte_offset(msgnode->message, new_cursor_pos);

    // Находим длину удаляемого
    int del_len = byte_offset - byte_offset_prev;

    // Выделяем память для удаляемого символа и нуль-терминатора
    char* del = malloc(del_len + 1);
    if (!del) {
        perror("Failed to allocate memory for deleted");
        return NULL;
    }

    // Копируем удаляемое в эту память
    strncpy(del, msgnode->message + byte_offset_prev, del_len);

    // Завершаем удаляемое нуль-терминатором
    memset(del + del_len, 0, 1);

    // Вычисляем новую длину сообщения в байтах
    int new_length = strlen(msgnode->message) - del_len + 1;

    // Выделяем память под новое сообщение
    char* new_message = malloc(new_length);
    if (!new_message) {
        perror("Не удалось выделить память");
        return NULL;
    }

    // Копирование части строки до предыдущего символа
    strncpy(new_message, msgnode->message, byte_offset_prev);

    // Копирование части строки после текущего символа
    strcpy(new_message + byte_offset_prev,
           msgnode->message + byte_offset);

    // Освобождаем старое сообщение и присваиваем новое
    free(msgnode->message);
    msgnode->message = new_message;

    // Обновляем позицию курсора
    msgnode->cursor_pos = new_cursor_pos;

    // Теперь, когда у нас есть изменненная msgnode
    // мы можем создать State для undoStack
    State* currState = createState(msgnode, event);

    // Запоминить в состоянии удаляемый символ
    currState->seq = del;

    // [TODO:gmm] Надо запоминать несколько удаляемых символов
    // которые могут быть разной длины, т.к. utf-8

    return currState;
}

State* cmd_backward_char(MsgNode* msgnode, InputEvent* event) {
    if ( (!msgnode) || (!msgnode->message)
         || (msgnode->cursor_pos == 0) ) {
        // Если с msgnode что-то не так,
        // отпустим lock и вернемся
        return NULL;
    }

    // Если двигать назад уже некуда, то вернемся
    if (msgnode->cursor_pos == 0) {
        return NULL;
    }

    // Перемещаем курсор
    if (msgnode->cursor_pos > 0) {
        msgnode->cursor_pos--;
    }

    // Теперь, когда у нас есть изменненная msgnode
    // мы можем заняться State для undoStack.

    // Указатель на последнее состояние в undo-стеке
    // либо мы его создадим либо возьмем с вершины стека
    State* currState = NULL;

    // Пытаемся получить предыдущее состояние из undo-стека
    State* prevState = popState(&undoStack);
    if (!prevState) {
        // Если не получилось получить предыдущее состояние
        // то формируем новое состояние, которое будет возвращено
        currState = createState(msgnode, event);
    } else if (prevState->cmdFn == cmd_backward_char) {
        // Если предыдущее состояние такое же
        // - будем считать текущим состояние, которое
        // мы взяли с вершины undo-стека
        currState = prevState;
        // Увеличим cnt в текущем состоянии
        currState->cnt++;
    } else if (prevState->cmdFn == cmd_forward_char) {
        if (prevState->cnt > 1) {
            // И оно имеет несколько повторений
            // - то уменьшим ему кол-во повторений
            prevState->cnt--;
            // и вернем его обратно
            pushState(&undoStack, prevState);
        } else {
            // Если оно имеет 1 повтороение
            // то просто не возвращаем его в undo стек
            // и таким образом два состояния (текущее и предыдущее)
            // рекомбинируются - т.е. мы ничего не делаем здесь
        }
    } else {
        // Если предыдущее состояние какое-то другое
        // - вернем его обратно
        pushState(&undoStack, prevState);
        // и сформируем для возврата новое состояние
        currState = createState(msgnode, event);
    }

    return currState;
}

State* cmd_forward_char(MsgNode* msgnode, InputEvent* event) {
    if ( (!msgnode) || (!msgnode->message) ) {
        // Если с msgnode что-то не так, то вернемся
        return NULL;
    }

    // Вычислим максимальную позицию курсора
    int len = utf8_strlen(msgnode->message);

    // Если двигать вперед уже некуда, то вернемся
    if (msgnode->cursor_pos >= len) {
        return NULL;
    }

    // Перемещаем курсор (">=", чтобы позволить курсору
    // стоять на позиции нулевого терминирующего символа
    if (++msgnode->cursor_pos >= len) {
        msgnode->cursor_pos = len;
    }

    // Теперь, когда у нас есть изменненная msgnode
    // мы можем  заняться State для undoStack.

    // Указатель на последнее состояние в undo-стеке
    // либо мы его создадим либо возьмем с вершины стека
    State* currState = NULL;

    // Пытаемся получить предыдущее состояние из undo-стека
    State* prevState = popState(&undoStack);
    if (!prevState) {
        // Если не получилось получить предыдущее состояние
        // то формируем новое состояние, которое и будет возвращено
        currState = createState(msgnode, event);
    } else if (prevState->cmdFn == cmd_forward_char) {
        // Если предыдущее состояние такое же
        // - будем считать текущим состояние, которое
        // мы взяли с вершины undo-стека
        currState = prevState;
        // Увеличим cnt в текущем состоянии
        currState->cnt++;
    } else if (prevState->cmdFn == cmd_backward_char) {
        // Если предыдущее состояние - противоположное
        if (prevState->cnt > 1) {
            // И оно имеет несколько повторений
            // - то уменьшим ему кол-во повторений
            prevState->cnt--;
            // и вернем его обратно
            pushState(&undoStack, prevState);
        } else {
            // Если оно имеет 1 повтороение
            // то просто не возвращаем его в undo стек
            // и таким образом два состояния (текущее и предыдущее)
            // рекомбинируются - т.е. мы ничего не делаем здесь
        }
    } else {
        // Если предыдущее состояние какое-то другое
        // - вернем его обратно
        pushState(&undoStack, prevState);
        // и сформируем для возврата новое состояние
        currState = createState(msgnode, event);
    }

    return currState;
}

// Перемещение курсора вперед на одно слово
State* cmd_forward_word(MsgNode* node, InputEvent* event) {
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

    return NULL;
}

// Перемещение курсора назад на одно слово
State* cmd_backward_word(MsgNode* node, InputEvent* event) {
    if (node->cursor_pos == 0) return NULL;  // Если курсор уже в начале, ничего не делаем

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

    return NULL;
}

State* cmd_to_beginning_of_line(MsgNode* node, InputEvent* event) {
    // Если курсор уже в начале текста, ничего не делаем
    if (node->cursor_pos == 0) return NULL;

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Движемся назад, пока не найдем начало строки или начало текста
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(node->message, byte_offset);
        if (node->message[prev_offset] == '\n') {
            // Переходим на следующий символ после \n
            byte_offset = utf8_next_char(node->message, prev_offset);
            node->cursor_pos = utf8_strlen(node->message);
            return  NULL;
        }
        byte_offset = prev_offset;
        node->cursor_pos--;
    }
    // Если достигли начала текста, устанавливаем курсор на позицию 0
    node->cursor_pos = 0;

    return NULL;
}

State* cmd_to_end_of_line(MsgNode* node, InputEvent* event) {
    // Длина строки в байтах
    int len = strlen(node->message);

    // Смещение в байтах от начала строки до курсора
    int byte_offset = utf8_byte_offset(node->message, node->cursor_pos);

    // Движемся вперед, пока не найдем конец строки или конец текста
    while (byte_offset < len) {
        if (node->message[byte_offset] == '\n') {
            node->cursor_pos = utf8_char_index(node->message, byte_offset);
            return NULL;
        }
        byte_offset = utf8_next_char(node->message, byte_offset);
        node->cursor_pos++;
    }

    // Если достигли конца текста
    node->cursor_pos = utf8_char_index(node->message, len);

    return NULL;
}

State* cmd_prev_msg() {
    moveToPreviousMessage(&msgList);

    return NULL;
}

State* cmd_next_msg() {
    moveToNextMessage(&msgList);

    return NULL;
}

// Функция для вставки текста в позицию курсора
State* cmd_insert(MsgNode* msgnode, InputEvent* event) {
    if (!msgnode || !msgnode->message) {
        // Если с msgnode что-то не так, то вернемся
        return NULL;
    }

    // Байтовое смещение позиции курсора
    int byte_offset =
        utf8_byte_offset(msgnode->message, msgnode->cursor_pos);

    // Длина вставляемой последовательности
    int insert_len = strlen(event->seq);

    // Реаллоцируем message
    char* new_message =
        realloc(msgnode->message,
                strlen(msgnode->message) + insert_len + 1);
    if (new_message == NULL) {
        // Тут Lock можно не отпускать, все равно выходим
        perror("Failed to reallocate memory");
        exit(1);
    }

    // Обновляем указатель на message в msgnode
    msgnode->message = new_message;

    // Сдвиг части строки справа от курсора вправо на insert_len
    memmove(msgnode->message + byte_offset + insert_len, // dest
            msgnode->message + byte_offset,              // src
            strlen(msgnode->message) - byte_offset + 1); // size_t

    // Вставка event->seq в позицию курсора
    memcpy(msgnode->message + byte_offset, event->seq, insert_len);

    // Смещение курсора (выполняется в utf8-символах)
    msgnode->cursor_pos += utf8_strlen(event->seq);

    // Указатель на последнее состояние в undo-стеке
    // либо мы его создадим либо возьмем с вершины стека
    State* currState = NULL;

    // Пытаемся получить предыдущее состояние из undo-стека
    State* prevState = popState(&undoStack);
    if (!prevState) {
        // Если не получилось получить предыдущее состояние
        // то формируем новое состояние, которое будет возвращено
        currState = createState(msgnode, event);
    } else if ( (prevState->cmdFn != cmd_insert)
                || (utf8_char_length(prevState->seq) > 5) ) {
        // Если предыдущее состояние не cmd_insert
        // или его длина больше 5, вернем его обратно
        pushState(&undoStack, prevState);
        //  и сформируем для возврата новое состояние
        currState = createState(msgnode, event);
    } else {
        // Иначе, если в undo-стеке существует предыдущее
        // состояние, которое тоже cmd_insert и в нем
        // менее 5 символов -  будем считать текущим
        // состояние, которое мы взяли с вершины undo-стека
        currState = prevState;

        // Вычислим длину insert-sequence в байтах
        int insert_seq_len  = strlen(currState->seq);
        // Вычислим длину в байтах того что хотим добавить
        int event_seq_len = strlen(event->seq);
        // Получится общая длина + терминатор
        int new_seq_len = insert_seq_len + event_seq_len + 1;

        // Реаллоцируем seq в new_seq
        char* new_seq =
            realloc(currState->seq, new_seq_len);
        if (new_seq == NULL) {
            perror("Failed to reallocate memory");
            exit(1);
        }

        // Записываем завершающий нуль, на всякий
        memset(new_seq + new_seq_len - 2, 0, 1);

        // Дописываем к new_seq новый event->seq (в конец)
        strncpy(new_seq + insert_seq_len,
                event->seq, event_seq_len);

        // Записываем реаллоцированный seq в currState
        currState->seq = new_seq;

        // Отладочный вывод
        /* char log[DBG_LOG_MSG_SIZE] = {0}; */
        /* snprintf( */
        /*     log, sizeof(log), */
        /*     "insert_seq_len:%d event_seq_len:%d, new_seq_len:%d (%p -> %p) <%s>", */
        /*     insert_seq_len, event_seq_len, new_seq_len, */
        /*     currState->seq, new_seq, new_seq); */
        /* pushMessage(&msgList, log); */
    }

    // Возвращаем State
    return currState;
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
    int start = min(node->cursor_pos, node->shadow_cursor_pos);
    int end = max(node->cursor_pos, node->shadow_cursor_pos);

    int start_byte = utf8_byte_offset(node->message, start);
    int end_byte = utf8_byte_offset(node->message, end);

    if (start_byte != end_byte) {
        char* text_to_copy = strndup(node->message + start_byte, end_byte - start_byte);
        push_clipboard(text_to_copy);
        free(text_to_copy);
    }

    return NULL;
}

State* cmd_cut(MsgNode* node, InputEvent* event) {
    /* cmd_copy(node, param); // Сначала копируем текст */

    /* int start = min(node->cursor_pos, node->shadow_cursor_pos); */
    /* int end = max(node->cursor_pos, node->shadow_cursor_pos); */

    /* int start_byte = utf8_byte_offset(node->message, start); */
    /* int end_byte = utf8_byte_offset(node->message, end); */

    /* memmove(node->message + start_byte, node->message + end_byte, */
    /*         strlen(node->message) - end_byte + 1); */
    /* node->cursor_pos = start; */
    /* node->shadow_cursor_pos = start; */

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

State* cmd_toggle_cursor_shadow(MsgNode* node, InputEvent* event) {
    int temp = node->cursor_pos;
    node->cursor_pos = node->shadow_cursor_pos;
    node->shadow_cursor_pos = temp;

    return NULL;
}

State* cmd_undo(MsgNode* msgnode, InputEvent* event) {
    if (undoStack == NULL) {
        pushMessage(&msgList, "No more actions to undo");
        // Если undo-стек пуст, то ничего делать не надо
        return NULL;
    } else {
        // Извлекаем последнее состояние из undoStack
        State* prevState = popState(&undoStack);

        if (!prevState) {
            pushMessage(&msgList, "Failed to pop state from undoStack");
            return NULL;
        }

        // Отладочный вывод
        char logMsg[DBG_LOG_MSG_SIZE] = {0};
        int len = 0;
        if (prevState->seq) {
            len = (int)utf8_strlen(prevState->seq);
        }
        snprintf(
            logMsg,
            sizeof(logMsg),
            "cmd_undo: %s - <%s>(%d) [%d]",
            descr_cmd_fn(prevState->cmdFn),
            prevState->seq,
            len,
            prevState->cnt
            );
        pushMessage(&msgList, logMsg);

        if (prevState->cmdFn == cmd_insert) {
            // Вычисляем сколько utf-8 символов надо удалить
            int cnt_del = utf8_strlen(prevState->seq);
            // Вычисляем сколько они занимают байт
            int ins_len = strlen(prevState->seq);

            // Находим байтовую позицию символа под курсором
            int byte_offset =
                utf8_byte_offset(msgnode->message,
                                 msgnode->cursor_pos);

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
            strcpy(msgnode->message + new_byte_offset,
                   msgnode->message + byte_offset);
            // Реаллоцируем строку [TODO:gmm] segfault
            /* msgnode->message = */
            /*     realloc(msgnode->message, */
            /*             strlen(msgnode->message) */
            /*             - ins_len); */
            //

            // Перемещаем курсор на кол-во удаленных символов
            msgnode->cursor_pos -= cnt_del;

            // Перемещаем prevState в redoStack
            pushState(&redoStack, prevState);
        } else if (prevState->cmdFn == cmd_backward_char) {
            // Создаем event
            InputEvent* event = malloc(sizeof(InputEvent));
            event->type = CMD;
            event->cmdFn = cmd_forward_char;
            /* if (prevState->seq) { */
            /*     event->seq = strdup(prevState->seq); */
            /* } else { */
                event->seq = NULL;
            /* } */
            // Применяем event к msgList.current
            // prevState->cnt раз.
            // Возвращаемое значение не будет помещено в
            // undo стек, т.к. это cmd_undo (особый случай)
            for (int i=0; i < prevState->cnt; i++) {
                event->cmdFn(msgnode, event);
            }
            // Перемещаем prevState в redoStack
            pushState(&redoStack, prevState);
        } else if (prevState->cmdFn == cmd_forward_char) {
            // Создаем event
            InputEvent* event = malloc(sizeof(InputEvent));
            event->type = CMD;
            event->cmdFn = cmd_backward_char;
            /* if (prevState->seq) { */
            /*     event->seq = strdup(prevState->seq); */
            /* } else { */
            event->seq = NULL;
            /* } */
            // Применяем event к msgList.current
            // prevState->cnt раз.
            // Возвращаемое значение не будет помещено в
            // undo стек, т.к. это cmd_undo (особый случай)
            for (int i=0; i < prevState->cnt; i++) {
                event->cmdFn(msgnode, event);
            }
            // Перемещаем prevState в redoStack
            pushState(&redoStack, prevState);
        } else {
            // ...
            // Перемещаем prevState в redoStack
            pushState(&redoStack, prevState);
        }

        // Возвращаем предыдущее состояние чтобы обновить отображение
        return prevState;
    }
}

State* cmd_redo(MsgNode* msgnode, InputEvent* event) {
    if (redoStack == NULL) {
        pushMessage(&msgList, "No more actions to redo");
        // Если redoStack пуст, то ничего перерисовывать не надо
        return NULL;
    } else {

        // Извлекаем состояние из redoStack
        State* prevState = popState(&redoStack);

        if (!prevState) {
            pushMessage(&msgList, "Failed to pop state from redoStack");
            return NULL;
        } else {
            // [TODO:gmm] Надо завести набор парных функций
            // undo/redo и искать обратную функцию в этом наборе
            if ( (prevState->cmdFn == cmd_insert)
                 || (prevState->cmdFn == cmd_backward_char)
                 || (prevState->cmdFn == cmd_forward_char) ) {
                // Создаем event
                InputEvent* event = malloc(sizeof(InputEvent));
                event->type = CMD;
                event->cmdFn = prevState->cmdFn;
                if (prevState->seq) {
                    event->seq = strdup(prevState->seq);
                } else {
                    event->seq = NULL;
                }
                // Применяем event к msgList.current
                // prevState->cnt раз.
                // Возвращаемое значение не будет помещено в
                // undo стек, т.к. это cmd_undo (особый случай)
                for (int i=0; i < prevState->cnt; i++) {
                    prevState->cmdFn(msgnode, event);
                }
            } else if (prevState->cmdFn == cmd_backspace) {
                // [TODO:gmm] восстановить запомненный удаленный символ
            } else {
                pushMessage(&msgList, "cmd_redo: unk_cmdFn");
            }
            // Перемещаем prevState в undoStack
            pushState(&undoStack, prevState);
        }

        // Возвращаем состояние чтобы обновить отображение
        return prevState;
    }
}
