// msg.h
#ifndef MSG_H
#define MSG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct MsgNode {
    char* text;
    struct MsgNode* prev;
    struct MsgNode* next;
    int cursor_pos;
    int shadow_cursor_pos;
} MsgNode;

typedef struct {
    MsgNode* head;
    MsgNode* tail;
    MsgNode* curr;
    int size;
} MsgList;

extern pthread_mutex_t msgList_mutex;

void initMsgList(MsgList* list);
void pushMessage(MsgList* list, const char* text);
void clearMsgList(MsgList* list);
void moveToNextMessage(MsgList* list);
void moveToPreviousMessage(MsgList* list);

#endif
