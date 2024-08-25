// term.h

#ifndef TERM_H
#define TERM_H

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void disableRawMode();
void enableRawMode();
void clearScreen();
void moveCursor(int x, int y);

#endif
