// msg.c

#include "msg.h"

void initMsgList(MsgList* list) {
    *list = (MsgList){0};
}

void pushMessage(MsgList* list, const char* text) {
    pthread_mutex_lock(&msgList_mutex);

    MsgNode* newNode = (MsgNode*)malloc(sizeof(MsgNode));
    if (newNode == NULL) {
        perror("Failed to allocate memory for new message node");
        pthread_mutex_unlock(&msgList_mutex);
        exit(1);
    }

    newNode->text = strdup(text);
    if (newNode->text == NULL) {
        free(newNode);
        perror("Failed to duplicate message string");
        pthread_mutex_unlock(&msgList_mutex);
        return;
    }

    newNode->prev = NULL;
    newNode->next = list->head;
    newNode->cursor_pos = 0;
    newNode->shadow_cursor_pos = 0;

    if (list->head) {
        list->head->prev = newNode;
    } else {
        list->tail = newNode;
    }

    list->head = newNode;

    if (list->curr == NULL) {
        list->curr = newNode;
    }

    list->size++;

    pthread_mutex_unlock(&msgList_mutex);
}

void clearMsgList(MsgList* list) {
    MsgNode* current = list->head;
    while (current != NULL) {
        MsgNode* temp = current;
        current = current->next;
        free(temp->text); // Освобождение памяти выделенной под строку
        free(temp);       // Освобождение памяти выделенной под узел
    }
    list->head = NULL;
    list->tail = NULL;
    list->curr = NULL;
    list->size = 0;
}

void moveToNextMessage(MsgList* list) {
    /* pthread_mutex_lock(&msgList_mutex); */
    if (list->curr && list->curr->next) {
        list->curr = list->curr->next;
    }
    /* pthread_mutex_unlock(&msgList_mutex); */
}

void moveToPreviousMessage(MsgList* list) {
    /* pthread_mutex_lock(&msgList_mutex); */
    if (list->curr && list->curr->prev) {
        list->curr = list->curr->prev;
    }
    /* pthread_mutex_unlock(&msgList_mutex); */
}
