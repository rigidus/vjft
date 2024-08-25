// msg.h
#ifndef MSG_H
#define MSG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct MessageNode {
    char* message;
    struct MessageNode* prev;
    struct MessageNode* next;
    int cursor_pos;
} MessageNode;

typedef struct {
    MessageNode* head;
    MessageNode* tail;
    MessageNode* current;
    int size;
} MessageList;

extern pthread_mutex_t messageList_mutex;

void initMessageList(MessageList* list);
void pushMessage(MessageList* list, const char* text);
void clearMessageList(MessageList* list);
void moveToNextMessage(MessageList* list);
void moveToPreviousMessage(MessageList* list);

#endif
