// term.h

#ifndef TERM_H
#define TERM_H

#include "all_libs.h"

void disableRawMode();
void enableRawMode();
void clearScreen();
void moveCursor(int x, int y);

#endif
