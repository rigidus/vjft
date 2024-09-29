// event.c

#include "event.h"
#include <string.h>

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
    if (cmd_fn == cmd_undo) return "cmd_undo";
    if (cmd_fn == cmd_redo) return "cmd_redo";
    if (cmd_fn == cmd_to_beginning_of_line) return "cmd_to_beginning_of_line";
    if (cmd_fn == cmd_to_end_of_line) return "cmd_to_end_of_line";
    if (cmd_fn == cmd_backward_word) return "cmd_backward_word";
	if (cmd_fn == cmd_forward_word) return "cmd_forward_word";
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

State* cmd_connect(MsgNode* msg, InputEvent* event) {
    connect_to_server("127.0.0.1", 8888);

    return NULL;
}

State* cmd_alt_enter(MsgNode* msgnode, InputEvent* event) {
    /* /\* pushMessage(&msgList, "ALT enter function"); *\/ */
    /* if (!msgnode || !msgnode->text) return NULL; */

    /* // Вычисляем byte_offset, используя текущую позицию курсора */
    /* int byte_offset = */
    /*     utf8_byte_offset(msgnode->text, msgnode->cursor_pos); */

    /* // Выделяем новую память для сообщения */
    /* // +2 для новой строки и нуль-терминатора */
    /* char* new_message = malloc(strlen(msgnode->text) + 2); */
    /* if (!new_message) { */
    /*     perror("Failed to allocate memory for new message"); */
    /*     return NULL; */
    /* } */

    /* // Копируем часть сообщения до позиции курсора */
    /* strncpy(new_message, msgnode->text, byte_offset); */
    /* new_message[byte_offset] = '\n';  // Вставляем символ новой строки */

    /* // Копируем оставшуюся часть сообщения */
    /* strcpy(new_message + byte_offset + 1, msgnode->text + byte_offset); */

    /* // Освобождаем старое сообщение и обновляем указатель */
    /* free(msgnode->text); */
    /* msgnode->text = new_message; */

    /* // Перемещаем курсор на следующий символ после '\n' */
    /* msgnode->cursor_pos++; */

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
    /* const char* text = msgnode->text; */
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

// [TODO:gmm] Мы можем делать вложенный undo если подменять
// стеки объектом, который тоже содержит стек

// [TODO:gmm] Функция для удаления символа справа от курсора

// Функция для удаления символов слева от курсора
State* cmd_backspace(MsgNode* msgnode, InputEvent* event) {
    if ( (!msgnode) || (!msgnode->text) ) {
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

    // Находим байтовую позицию текущего и предыдущего UTF-8 символов
    int byte_offset =
        utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

    // Вычисляем новую позицию курсора
    int new_cursor_pos = msgnode->cursor_pos - cnt_del;
    if (new_cursor_pos < 0) {
        new_cursor_pos = 0;
    }

    // Находим байтовую позицию предыдущего символа
    int byte_offset_prev =
        utf8_byte_offset(msgnode->text, new_cursor_pos);

    // Находим длину удаляемого символа в байтах
    int del_len = byte_offset - byte_offset_prev;

    // Указатель на последнее состояние в undo-стеке
    // либо мы его создадим либо возьмем с вершины стека
    // оно также служит нам флагом - если мы работаем
    // с состоянием из стека оно не будет NULL
    State* currState = NULL;

    // Здесь будет указатель на удаляемое
    char* del = NULL;

    // Пытаемся получить предыдущее состояние из undo-стека
    State* prevState = popState(&undoStack);
    if ( (prevState)
         && (prevState->cmdFn == cmd_backspace)
        ) {
        // Если предыдущее состояние существует и его команда
        // такая же, которую мы сейчас выполняем, то будем
        // присоединять удаляемое к предыдущему состоянию
        currState = prevState;

        // Определяем, сколько байт содержит currState->seq
        int old_len = strlen(currState->seq);

        // Реаллоцируем currState->seq чтобы вместить туда
        // новое удаляемое и нуль-терминатор
        del = realloc(currState->seq, old_len + del_len + 1);

        // Сдвигаем содержимое currState->seq вправо на del_len
        strncpy(currState->seq + del_len, currState->seq, old_len);

        // Дописываем в начало del удаляемое
        strncpy(del, msgnode->text + byte_offset_prev, del_len);

        // Завершаем удаляемое нуль-терминатором
        memset(del + old_len + del_len, 0, 1);

        // Обновляем currState->seq
        currState->seq = del;

    } else {
        /* // Предыдущее состояние нам не подходит, */
        /* // вернем его обратно. currState сейчас NULL */
        pushState(&undoStack, prevState);

        // :::: Формирование строки, содержащей удаляемое

        // Выделяем память для удаляемого символа и нуль-терминатора
        del = malloc(del_len + 1);
        if (!del) {
            perror("Failed to allocate memory for deleted");
            return NULL;
        }

        // Копируем удаляемое в эту память
        strncpy(del, msgnode->text + byte_offset_prev, del_len);

        // Завершаем удаляемое нуль-терминатором
        memset(del + del_len, 0, 1);
    }

    // :::: Изменение msgnode

    // Вычисляем новую длину text в байтах
    int new_len = strlen(msgnode->text) - del_len + 1;

    // Выделяем память под новый text
    char* new_text = malloc(new_len);
    if (!new_text) {
        perror("Не удалось выделить память");
        return NULL;
    }

    // Копирование части строки до предыдущего символа
    strncpy(new_text, msgnode->text, byte_offset_prev);

    // Копирование части строки после текущего символа
    strcpy(new_text + byte_offset_prev,
           msgnode->text + byte_offset);

    // Освобождаем старый text и присваиваем новый
    free(msgnode->text);
    msgnode->text = new_text;

    // Обновляем позицию курсора
    msgnode->cursor_pos = new_cursor_pos;

    // :::: Создание нового элемента undoStack

    if (!currState) {
        // Мы здесь, если не работали с состоянием из undo стека
        // Теперь, когда у нас есть измененная msgnode
        // мы можем создать State для undoStack
        currState = createState(msgnode, event);

        // Запоминить в его seq удаляемый символ
        currState->seq = del;
    }

    // Вернуть currState
    return currState;
}

State* cmd_backward_char(MsgNode* msgnode, InputEvent* event) {
    if ( (!msgnode) || (!msgnode->text)
         || (msgnode->cursor_pos == 0) ) {
        // Если с msgnode что-то не так, то вернемся
        return NULL;
    }

    // Если двигать влево уже некуда, то вернемся
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
    if ( (!msgnode) || (!msgnode->text) ) {
        // Если с msgnode что-то не так, то вернемся
        return NULL;
    }

    // Вычислим максимальную позицию курсора
    int len = utf8_strlen(msgnode->text);

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
State* cmd_forward_word(MsgNode* msgnode, InputEvent* event) {
    int len = utf8_strlen(msgnode->text);  // Длина строки в utf-8 символах

	if (msgnode->cursor_pos == len) {
        // Если курсор уже в конце строки, ничего не делаем
        return NULL;
    }

    // Делаем из msgnode->cursor_pos строку,
    // которую позже запишем в currState->seq
    char* seq = int_to_hex(msgnode->cursor_pos);

	// Создаем State
    State* currState = createState(msgnode, event);

    // Записываем в него старую позицию курсора
    currState->seq = seq;

    // Смещение в байтах от начала строки до курсора
    int byte_offset =
		utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

	// Движемся вперед по utf-8 символам
    // Пропуск пробелов (если курсор находится на пробеле)
    while ( (byte_offset < len)
			&& isspace(
				(unsigned char) msgnode->text[byte_offset])) {
        byte_offset =
			utf8_next_char(msgnode->text, byte_offset);
        msgnode->cursor_pos++;
    }

    // Перемещение вперед до конца слова
    while ( (byte_offset < len)
		   && !isspace(
			   (unsigned char)msgnode->text[byte_offset]) ){
        byte_offset =
			utf8_next_char(msgnode->text, byte_offset);
        msgnode->cursor_pos++;
    }

    return currState;
}

// Перемещение курсора назад на одно слово
State* cmd_backward_word(MsgNode* msgnode, InputEvent* event) {
    if (msgnode->cursor_pos == 0) {
        // Если курсор уже в начале строки, ничего не делаем
        return NULL;
    }

    // Делаем из msgnode->cursor_pos строку,
    // которую позже запишем в currState->seq
    char* seq = int_to_hex(msgnode->cursor_pos);

    // Создаем State
    State* currState = createState(msgnode, event);

    // Записываем в него старую позицию курсора
    currState->seq = seq;

    // Смещение в байтах от начала строки до курсора
    int byte_offset =
        utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

    // Пропуск пробелов, если курсор находится на пробеле
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(msgnode->text, byte_offset);
        if (!isspace((unsigned char)msgnode->text[prev_offset])) {
            break;
        }
        byte_offset = prev_offset;
        msgnode->cursor_pos--;
    }

    // Перемещение назад до начала предыдущего слова
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(msgnode->text, byte_offset);
        if (isspace((unsigned char)msgnode->text[prev_offset])) {
            break;
        }
        byte_offset = prev_offset;
        msgnode->cursor_pos--;
    }

    return currState;
}

State* cmd_to_beginning_of_line(MsgNode* msgnode, InputEvent* event) {
    // Если двигать влево уже некуда, то вернемся
    if (msgnode->cursor_pos == 0) {
        return NULL;
    }

    // Делаем из msgnode->cursor_pos строку,
    // которую позже запишем в currState->seq
    char* seq = int_to_hex(msgnode->cursor_pos);

    // Создаем State
    State* currState = createState(msgnode, event);

    // Записываем в него старую позицию курсора
    currState->seq = seq;

    // Смещение в байтах от начала строки до курсора
    int byte_offset =
        utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

    // Движемся назад, пока не найдем начало строки или начало текста
    while (byte_offset > 0) {
        int prev_offset = utf8_prev_char(msgnode->text, byte_offset);
        if (msgnode->text[prev_offset] == '\n') {
            // Переходим на следующий символ после '\n'
            byte_offset = utf8_next_char(msgnode->text, prev_offset);
            msgnode->cursor_pos = utf8_strlen(msgnode->text);
            return currState;
        }
        byte_offset = prev_offset;
        msgnode->cursor_pos--;
    }
    // Если достигли начала текста, устанавливаем курсор на позицию 0
    msgnode->cursor_pos = 0;

    return currState;
}

State* cmd_get_back_to_line_pos(MsgNode* msgnode, InputEvent* event) {
    if (!msgnode || !msgnode->text) {
        // Если с msgnode что-то не так, то вернемся
        return NULL;
    }

    // seq хранит старую позицию курсора в шестнадцатеричном виде
    int old_cursor_pos = hex_to_int(event->seq);

    // Проверка на выход за пределы допустимого
    if ( (old_cursor_pos < 0)
         || (old_cursor_pos > utf8_strlen(msgnode->text)) ) {
        return NULL;
    }

    // Восстанавливаем старую позицию курсора
    msgnode->cursor_pos = old_cursor_pos;

    return NULL;
}


State* cmd_to_end_of_line(MsgNode* msgnode, InputEvent* event) {
    if ( (!msgnode) || (!msgnode->text) ) {
        // Если с msgnode что-то не так, то вернемся
        return NULL;
    }

    // Если двигать вправо уже некуда, то вернемся
    if (msgnode->cursor_pos >= utf8_strlen(msgnode->text)) {
        return NULL;
    }

    // Делаем из msgnode->cursor_pos строку,
    // которую позже запишем в currState->seq
    char* seq = int_to_hex(msgnode->cursor_pos);

    // Создаем State
    State* currState = createState(msgnode, event);

    // Записываем в него старую позицию курсора
    currState->seq = seq;

    // Длина строки в байтах
    int len = strlen(msgnode->text);

    // Смещение в байтах от начала строки до курсора
    int byte_offset =
        utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

    // Движемся вперед, пока не найдем конец строки или конец текста
    while (byte_offset < len) {
        if (msgnode->text[byte_offset] == '\n') {
            msgnode->cursor_pos =
                utf8_char_index(msgnode->text, byte_offset);
            return currState;
        }
        byte_offset = utf8_next_char(msgnode->text, byte_offset);
        msgnode->cursor_pos++;
    }

    // Если достигли конца текста
    msgnode->cursor_pos = utf8_char_index(msgnode->text, len);

    return currState;
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
    if (!msgnode || !msgnode->text) {
        return NULL;
    }

    // Байтовое смещение позиции курсора
    int byte_offset =
        utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

    // Длина вставляемой последовательности в байтах
    int insert_len = strlen(event->seq);

    // Реаллоцируем message
    char* new_text =
        realloc(msgnode->text,
                strlen(msgnode->text) + insert_len + 1);
    if (new_text == NULL) {
        perror("Failed to reallocate memory");
        exit(1);
    }

    // Обновляем указатель на message в msgnode
    msgnode->text = new_text;

    // Сдвиг части строки справа от курсора вправо на insert_len
    memmove(msgnode->text + byte_offset + insert_len, // dest
            msgnode->text + byte_offset,              // src
            strlen(msgnode->text) - byte_offset + 1); // size_t

    // Вставка event->seq в позицию курсора
    memcpy(msgnode->text + byte_offset, event->seq, insert_len);

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
    /* int start = min(node->cursor_pos, node->shadow_cursor_pos); */
    /* int end = max(node->cursor_pos, node->shadow_cursor_pos); */

    /* int start_byte = utf8_byte_offset(msgnode->text, start); */
    /* int end_byte = utf8_byte_offset(msgnode->text, end); */

    /* if (start_byte != end_byte) { */
    /*     char* text_to_copy = strndup(msgnode->text + start_byte, end_byte - start_byte); */
    /*     push_clipboard(text_to_copy); */
    /*     free(text_to_copy); */
    /* } */

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

State* cmd_uninsert(MsgNode* msgnode, InputEvent* event) {
    // Вычисляем сколько utf-8 символов надо удалить
    int cnt_del = utf8_strlen(event->seq);

    // Вычисляем сколько они занимают байт
    int ins_len = strlen(event->seq);

    // Находим байтовую позицию символа под курсором
    int byte_offset =
        utf8_byte_offset(msgnode->text, msgnode->cursor_pos);

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
    strcpy(msgnode->text + new_byte_offset,
           msgnode->text + byte_offset);
    // Реаллоцируем строку [TODO:gmm] segfault
    /* msgnode->message = */
    /*     realloc(msgnode->message, */
    /*             strlen(msgnode->message) */
    /*             - ins_len); */
    //

    // Перемещаем курсор на кол-во удаленных символов
    msgnode->cursor_pos -= cnt_del;

    return NULL;
}

State* cmd_undo(MsgNode* msgnode, InputEvent* event) {
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
        undoCmd(msgnode, &undoEvent);
    }

    // Перемещаем prevState в redoStack
    pushState(&redoStack, prevState);

    // Clean up
    if (undoEvent.seq) free(undoEvent.seq);

    // Return the prevState to indicate the state has changed
    return prevState;
}

State* cmd_redo(MsgNode* msgnode, InputEvent* event) {
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
        nextState->cmdFn(msgnode, &redoEvent);
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
