// ctrlstk.c

#include "ctrlstk.h"

void pushCtrlStack(Key key) {
    CtrlStack* newElement = malloc(sizeof(CtrlStack));
    newElement->key = key;
    newElement->next = ctrlStack;
    ctrlStack = newElement;
}

Key popCtrlStack() {
    if (!ctrlStack) return '\0';
    Key key = ctrlStack->key;
    CtrlStack* temp = ctrlStack;
    ctrlStack = ctrlStack->next;
    free(temp);
    return key;
}

bool isCtrlStackEmpty() {
    return ctrlStack == NULL;
}

bool isCtrlStackActive(Key key) {
    // Проверьте стек на наличие данного ключа
    // Возвращает true, если ключ найден
    if (!ctrlStack) return false;
    if (ctrlStack->key == key) {
        return true;
    }
    return false;
}

void clearCtrlStack() {
    while (popCtrlStack());
}
