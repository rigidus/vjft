// ctrlstk.h

#include "all_libs.h"

#include "key.h"

typedef struct CtrlStack {
    Key key;
    struct CtrlStack* next;
} CtrlStack;

extern CtrlStack* ctrlStack;

void pushCtrlStack(Key key);
Key popCtrlStack();
bool isCtrlStackEmpty();
bool isCtrlStackActive(Key key);
void clearCtrlStack();
