// msg.c

#include "msg.h"

void initMessageList(MessageList* list) {
    *list = (MessageList){0};
}

void pushMessage(MessageList* list, const char* text) {
    pthread_mutex_lock(&messageList_mutex);

    MessageNode* newNode = (MessageNode*)malloc(sizeof(MessageNode));
    if (newNode == NULL) {
        perror("Failed to allocate memory for new message node");
        pthread_mutex_unlock(&messageList_mutex);
        exit(1);
    }

    newNode->message = strdup(text);
    if (newNode->message == NULL) {
        free(newNode);
        perror("Failed to duplicate message string");
        pthread_mutex_unlock(&messageList_mutex);
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

    if (list->current == NULL) {
        list->current = newNode;
    }

    list->size++;

    pthread_mutex_unlock(&messageList_mutex);
}

void clearMessageList(MessageList* list) {
    MessageNode* current = list->head;
    while (current != NULL) {
        MessageNode* temp = current;
        current = current->next;
        free(temp->message);  // Освобождение памяти выделенной под строку
        free(temp);           // Освобождение памяти выделенной под узел
    }
    list->head = NULL;
    list->tail = NULL;
    list->current = NULL;
    list->size = 0;
}

void moveToNextMessage(MessageList* list) {
    /* pthread_mutex_lock(&messageList_mutex); */
    if (list->current && list->current->next) {
        list->current = list->current->next;
    }
    /* pthread_mutex_unlock(&messageList_mutex); */
}

void moveToPreviousMessage(MessageList* list) {
    /* pthread_mutex_lock(&messageList_mutex); */
    if (list->current && list->current->prev) {
        list->current = list->current->prev;
    }
    /* pthread_mutex_unlock(&messageList_mutex); */
}
