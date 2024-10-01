// events.h

#ifndef EVENT_H
#define EVENT_H

#include "all_libs.h"

#include "msg.h"
#include "utf8.h"
#include "key.h"
/* #include "cmd.h" */

extern MsgList msgList;
extern pthread_mutex_t msgList_mutex;

// Предварительное объявление структуры Action
typedef struct Action Action;

// Предварительное объявление структуры InputEvent
typedef struct InputEvent InputEvent;

// Определение типа функции CmdFunc, который теперь
// знает о типе Action и InputEvent
typedef Action* (*CmdFunc)(MsgNode*, InputEvent* event);

// [TODO:gmm] Типы Event-ов нужны для отладочного вывода, после удалить
typedef enum { DBG, CMD } EventType;

// Полное определение структуры InputEvent
typedef struct InputEvent {
    EventType type; // [TODO:gmm] удалить после отладки
    CmdFunc cmdFn;
    char* seq;
    struct InputEvent* next;
} InputEvent;

// Полное определение структуры Action,
// которое уже может использовать CmdFunc
typedef struct Action {
    CmdFunc cmdFn;
    char* seq;
    int cnt;
} Action;

// Тип стека экшенов для undo/redo
typedef struct ActionStackElt {
    Action* act;
    struct ActionStackElt* next;
} ActionStackElt;

// Стеки undo и redo
extern ActionStackElt* undoStack;
extern ActionStackElt* redoStack;

// Очереди и их мьютексы
extern InputEvent* gInputEventQueue;
extern pthread_mutex_t gEventQueue_mutex;

extern InputEvent* gHistoryEventQueue;
extern pthread_mutex_t gHistoryQueue_mutex;

// Добавлятель в очередь
void enqueueEvent(InputEvent** eventQueue,
                  pthread_mutex_t* queueMutex,
                  EventType type, CmdFunc cmdFn, char* seq);


#define ASCII_CODES_BUF_SIZE 127
#define DBG_LOG_MSG_SIZE 255

void convertToAsciiCodes(const char *input, char *output,
                         size_t outputSize);

// Процессор событий
bool processEvents(InputEvent** eventQueue,
                   pthread_mutex_t* queueMutex,
                   char* input, int* input_size,
                   int* log_window_start, int rows);


// Функции работы с экшенами отмены/возврата
Action* createAction(MsgNode* currentNode, InputEvent* event);
void freeAction(Action* act);
void pushAction(ActionStackElt** stack, Action* act);
Action* popAction(ActionStackElt** stack);
void clearStack(ActionStackElt** stack);

// Функция для отладочного отображения CmdFunc
char* descr_cmd_fn(CmdFunc cmd_fn);

// Команды

void connect_to_server(const char* server_ip, int port);

char* int_to_hex(int value);
int hex_to_int(const char* hex);

Action* cmd_stub(MsgNode* msg, InputEvent* event);
Action* cmd_connect();
Action* cmd_enter(MsgNode* msg, InputEvent* event);
Action* cmd_alt_enter(MsgNode* msg, InputEvent* event);

Action* cmd_backward_char(MsgNode* node, InputEvent* event);
Action* cmd_forward_char(MsgNode* node, InputEvent* event);
Action* cmd_forward_word(MsgNode* node, InputEvent* event);
Action* cmd_backward_word(MsgNode* node, InputEvent* event);
Action* cmd_to_end_of_line(MsgNode* node, InputEvent* event);
Action* cmd_to_beginning_of_line(MsgNode* node, InputEvent* event);
Action* cmd_insert(MsgNode* node, InputEvent* event);
Action* cmd_backspace(MsgNode* msg, InputEvent* event);

Action* cmd_prev_msg();
Action* cmd_next_msg();

Action* cmd_copy(MsgNode* node, InputEvent* event);
Action* cmd_cut(MsgNode* node, InputEvent* event);
Action* cmd_paste(MsgNode* node, InputEvent* event);
Action* cmd_toggle_cursor(MsgNode* node, InputEvent* event);
Action* cmd_undo(MsgNode* msg, InputEvent* event);
Action* cmd_redo(MsgNode* msg, InputEvent* event);

Action* cmd_set_marker(MsgNode* node, InputEvent* event);
Action* cmd_unset_marker(MsgNode* node, InputEvent* event);

typedef struct {
    CmdFunc redo_cmd;
    CmdFunc undo_cmd;
} CmdPair;

extern CmdPair comp_cmds[];

CmdFunc get_comp_cmd (CmdFunc cmd);

#endif
