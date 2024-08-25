//term.c

#include "term.h"

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    // Сохраняем текущие настройки терминала
    tcgetattr(STDIN_FILENO, &orig_termios);
    // Гарантируем возврат к нормальным настройкам при выходе
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    // Отключаем эхо и канонический режим
    raw.c_lflag &= ~(ECHO | ICANON);

    // Отключаем эхо, канонический режим и сигналы
    /* raw.c_lflag &= ~(ECHO | ICANON | ISIG); */

    // Отключаем специальные управляющие символы
    /* raw.c_cc[VINTR] = 0;  // Ctrl-C */
    /* raw.c_cc[VQUIT] = 0;  // Ctrl-\ ; */
    /* raw.c_cc[VSTART] = 0; // Ctrl-Q */
    /* raw.c_cc[VSTOP] = 0;  // Ctrl-S */
    /* raw.c_cc[VEOF] = 0;   // Ctrl-D */
    /* raw.c_cc[VSUSP] = 0;  // Ctrl-Z */
    /* raw.c_cc[VREPRINT] = 0; // Ctrl-R */
    /* raw.c_cc[VLNEXT] = 0; // Ctrl-V */
    /* raw.c_cc[VDISCARD] = 0; // Ctrl-U */

    /* // Отключаем управление потоком */
    /* raw.c_iflag &= ~IXON; */
    /* /\* raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN); *\/ */
    /* /\* raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); *\/ */
    /* /\* raw.c_cflag |= (CS8); *\/ */
    /* /\* raw.c_oflag &= ~(OPOST); *\/ */
    /* // Minimum number of characters for noncanonical read (1) */
    /* raw.c_cc[VMIN] = 0; */
    /* // Timeout in deciseconds for noncanonical read (0.1 seconds) */
    /* raw.c_cc[VTIME] = 0; */

    // Устанавливаем raw mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Установка небуферизованного вывода для stdout
    /* setvbuf(stdout, NULL, _IONBF, 0); */
}

void clearScreen() {
    printf("\033[H\033[J");
    fflush(stdout);
}

void moveCursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}
