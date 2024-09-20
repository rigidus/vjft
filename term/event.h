// events.h

#ifndef EVENT_H
#define EVENT_H

#include "all_libs.h"

#include "msg.h"
#include "utf8.h"
#include "key.h"
/* #include "cmd.h" */

extern MessageList messageList;
extern pthread_mutex_t messageList_mutex;

// Предварительное объявление структуры State
typedef struct State State;

// Предварительное объявление структуры InputEvent
typedef struct InputEvent InputEvent;

// Определение типа функции CmdFunc, который теперь знает о типе State и InputEvent
typedef State* (*CmdFunc)(MessageNode*, InputEvent* event);

// Типы Event-ов (нужны для отладочного вывода, после удалить)
typedef enum {
    DBG,
    CMD
} EventType;

// Полное определение структуры InputEvent
typedef struct InputEvent {
    EventType type;
    CmdFunc cmdFn;
    char* seq;
    struct InputEvent* next;
} InputEvent;

// Полное определение структуры State,
// которое уже может использовать CmdFunc и InputEvent
typedef struct State {
    char* message;
    int cursor_pos;
    int shadow_cursor_pos;
    InputEvent forward;
    /* InputEvent revert; */
} State;

// Тип стека состояний для undo/redo
typedef struct StateStack {
    State* state;
    struct StateStack* next;
} StateStack;

// Стеки undo и redo
extern StateStack* undoStack;
extern StateStack* redoStack;

// Очереди и их мьютексы
extern InputEvent* gInputEventQueue;
extern pthread_mutex_t gEventQueue_mutex;

extern InputEvent* gHistoryEventQueue;
extern pthread_mutex_t gHistoryQueue_mutex;

// Добавлятель в очередь
void enqueueEvent(InputEvent** eventQueue, pthread_mutex_t* queueMutex,
                  EventType type, CmdFunc cmdFn, char* seq);


#define ASCII_CODES_BUF_SIZE 127
#define DBG_LOG_MSG_SIZE 255

void convertToAsciiCodes(const char *input, char *output,
                         size_t outputSize);

// Процессор событий
bool processEvents(InputEvent** eventQueue, pthread_mutex_t* queueMutex,
                   char* input, int* input_size,
                   int* log_window_start, int rows);


// Функции работы с состояниями отмены/возврата
State* createState(MessageNode* currentNode, InputEvent* event);
void freeState(State* state);
void pushState(StateStack** stack, State* state);
State* popState(StateStack** stack);
void clearStack(StateStack** stack);


// Команды

void connect_to_server(const char* server_ip, int port);

State* cmd_connect();
State* cmd_enter(MessageNode* msg, InputEvent* event);
State* cmd_alt_enter(MessageNode* msg, InputEvent* event);
State* cmd_backspace(MessageNode* msg, InputEvent* event);
State* cmd_prev_msg();
State* cmd_next_msg();
State* cmd_backward_char();
State* cmd_forward_char();
State* cmd_forward_word();
State* cmd_backward_word();
State* cmd_to_end_of_line();
State* cmd_to_beginning_of_line();
State* cmd_insert();
State* cmd_copy(MessageNode* node, InputEvent* event);
State* cmd_cut(MessageNode* node, InputEvent* event);
State* cmd_paste(MessageNode* node, InputEvent* event);
State* cmd_toggle_cursor_shadow(MessageNode* node, InputEvent* event);
State* cmd_undo(MessageNode* msg, InputEvent* event);
State* cmd_redo(MessageNode* msg, InputEvent* event);

#endif