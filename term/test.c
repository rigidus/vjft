#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>

// Глобальная переменная для хранения PID дочернего процесса
pid_t child_pid;

// Глобальная переменная для master_fd
int master_fd;

// Функция для установки терминала в raw-режим
void set_raw_mode(int fd) {
    struct termios term;
    if (tcgetattr(fd, &term) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    cfmakeraw(&term);
    if (tcsetattr(fd, TCSANOW, &term) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

// Структура для хранения тестовых случаев
struct test_case {
    const char *input;
    const char *expected_output;
};

// Массив тестовых случаев
struct test_case tests[] = {
    {"a", "expected output 1"},
    {"b", "expected output 2"},
    {"c", "expected output 3"},

    // Добавьте дополнительные тестовые случаи по необходимости
};


void update_window_size(int master_fd) {
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) {
        perror("ioctl TIOCGWINSZ");
        return;
    }
    if (ioctl(master_fd, TIOCSWINSZ, &ws) == -1) {
        perror("ioctl TIOCSWINSZ");
    }
}

// Обработчик сигнала SIGWINCH
void handle_sigwinch(int sig) {
    // Обновляем размер окна в pty
    update_window_size(master_fd);
    // Пересылаем SIGWINCH дочернему процессу
    if (child_pid > 0) {
        kill(child_pid, SIGWINCH);
    }
}

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}


int main() {
    int slave_fd;
    pid_t pid;
    char slave_name[100];
    struct termios orig_termios;

    // В функции main() установить обработчик сигнала
    signal(SIGWINCH, handle_sigwinch);

    // Создаем псевдотерминал
    if (openpty(&master_fd, &slave_fd, slave_name, NULL, NULL) == -1) {
        perror("openpty");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем размер окна для pty
    struct winsize ws;
    ws.ws_col = 110; // Задайте необходимое количество столбцов
    ws.ws_row = 35; // Задайте необходимое количество строк
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    if (ioctl(master_fd, TIOCSWINSZ, &ws) == -1) {
        perror("ioctl TIOCSWINSZ");
        exit(EXIT_FAILURE);
    }

    // Форкаем процесс
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Дочерний процесс
        close(master_fd);

        // Создаем новую сессию и устанавливаем управляющий терминал
        if (setsid() == -1) {
            perror("setsid");
            exit(EXIT_FAILURE);
        }
        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
            perror("ioctl TIOCSCTTY");
            exit(EXIT_FAILURE);
        }

        // Устанавливаем размер окна для slave_fd
        if (ioctl(slave_fd, TIOCSWINSZ, &ws) == -1) {
            perror("ioctl TIOCSWINSZ (child)");
            exit(EXIT_FAILURE);
        }

        // Устанавливаем терминальные атрибуты
        struct termios term;
        if (tcgetattr(slave_fd, &term) == -1) {
            perror("tcgetattr");
            exit(EXIT_FAILURE);
        }
        cfmakeraw(&term);
        if (tcsetattr(slave_fd, TCSANOW, &term) == -1) {
            perror("tcsetattr");
            exit(EXIT_FAILURE);
        }

        // Дублируем slave_fd на стандартные дескрипторы
        // ввода/вывода
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        close(slave_fd);

        // Запускаем целевую программу
        execl("./a.out", "a.out", (char *)NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Родительский процесс
        close(slave_fd);

        // Сохраняем возвращенный PID дочернего процесса
        child_pid = pid;

        // Сохраняем текущие настройки терминала и
        // переводим терминал в raw-режим
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
            perror("tcgetattr");
            exit(EXIT_FAILURE);
        }
        set_raw_mode(STDIN_FILENO);

        // Настраиваем master_fd на неблокирующий режим
        int flags = fcntl(master_fd, F_GETFL);
        if (fcntl(master_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }

        // Ждем чтобы программа успела проинициализироваться
        msleep(1000);

        size_t num_tests = sizeof(tests) / sizeof(tests[0]);
        char buffer[4096*10];
        for (size_t i = 0; i < num_tests; i++) {
            // Отправляем ввод в целевую программу
            write(master_fd, tests[i].input, strlen(tests[i].input));

            // Ждем и читаем вывод
            msleep(1000); // Ждем перед чтением вывода

            // Добавляем цикл для чтения данных из master_fd
            fd_set read_fds;
            struct timeval tv;
            int retval;

            FD_ZERO(&read_fds);
            FD_SET(master_fd, &read_fds);

            tv.tv_sec = 1; // Таймаут в секундах
            tv.tv_usec = 0;

            retval = select(master_fd + 1, &read_fds, NULL, NULL, &tv);
            if (retval == -1) {
                perror("select()");
            } else if (retval) {
                // Данные доступны для чтения
                ssize_t nbytes =
                    read(master_fd, buffer, sizeof(buffer) - 1);
                if (nbytes > 0) {
                    buffer[nbytes] = '\0';
                    // Обработка вывода
                    // Отображаем вывод
                    printf("%s", buffer);
                    // Сравниваем с ожидаемым результатом
                    if (strstr(buffer, tests[i].expected_output) == NULL)
                    {
                        fprintf(stderr,
                  "Тест %zu не пройден: ожидалось '%s', получено '%s'\n",
                                i + 1, tests[i].expected_output, buffer);
                        /* kill(pid, SIGKILL); // Прерываем целевую программу */
                        /* break; */
                    }
                }
            }
        }

        // Возвращаем терминал в исходный режим
        if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) == -1) {
            perror("tcsetattr");
            exit(EXIT_FAILURE);
        }

        // Ожидаем завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}
