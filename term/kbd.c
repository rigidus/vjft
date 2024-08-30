// kbd.c

#include "kbd.h"

#include "all_libs.h"

#include "utf8.h"

// Функция чтения одного UTF-8 символа или ESC последовательности в buf
int read_utf8_char_or_esc_seq(int fd, char* buf, int buf_size) {
    int len = 0, nread = 0;
    struct timeval timeout;
    fd_set read_fds;
    while (len < buf_size - 1) {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        // Сброс таймаута на каждой итерации
        timeout.tv_sec = 0; // 100 мс только после ESC
        timeout.tv_usec = (len == 1 && buf[0] == '\033') ? 100000 : 0;
        // Читаем
        nread = read(fd, buf + len, 1);
        if (nread < 0) return -1; // Ошибка чтения
        if (nread == 0) return 0; // Нет данных для чтения
        // Увеличиваем len и обеспечиваем нуль-терминирование для строки
        len++;
        buf[len] = '\0';
        // Проверка на начало escape-последовательности
        if (buf[0] == '\033') {
            // Если считан только первый символ и это ESC
            if (len == 1) {
                // Ждем, чтобы убедиться, что за ESC ничего не следует
                int select_res = select(fd + 1, &read_fds, NULL, NULL, &timeout);
                if (select_res == 0) {
                    // Тайм-аут истек, и других символов не было получено
                    break; // Только ESC был нажат
                }
                continue; // Есть еще данные для чтения
            }
            // Считано два символа и это CSI или SS3 последовательности
            // - читаем дальше
            if (len == 2 && (buf[1] == '[' || buf[1] == 'O')) { continue; }
            // Считано два символа, и второй из них - это \xd0 или \xd1
            // - читаем дальше
            // (это может быть модификатор + двухбайтовый кирилический символ)
            if (len == 2 && (buf[1] == '\xd0' || buf[1] == '\xd1')) { continue; }
            // Считано больше двух символов и это CSI или SS3 последовательности
            if (len > 2 && (buf[1] == '[' || buf[1] == 'O')) {
                // ...и при этом следующим cчитан алфавитный символ или тильда?
                if (isalpha(buf[len - 1]) || buf[len - 1] == '~') {
                    // последовательность завершена
                    break;
                } else {
                    // продолжаем считывать дальше
                    continue;
                }
            }
            // Тут мы окажемся только если после ESC пришло что-то неожиданное
            break;
        } else {
            // Это не escape-последовательность, а UTF-8 символ, который мы должны
            // прекратить читать только когда он будет complete
            if (is_utf8_complete(buf, len)) break;
        }
    }
    return len;
}
