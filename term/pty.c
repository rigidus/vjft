#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    int master_fd, slave_fd;

    // Создаем пару master-slave PTY
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) {
        perror("openpty");
        return EXIT_FAILURE;
    }

    printf("PTY успешно создан, master FD: %d, slave FD: %d\n", master_fd, slave_fd);

    int pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // В дочернем процессе
        setsid(); // Создаем новую сессию

        // Дублируем slave end PTY в стандартные потоки ввода/вывода/ошибок
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(master_fd);
        close(slave_fd);

        // Запускаем Midnight Commander
        execlp("mc", "mc", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // В родительском процессе
        close(slave_fd);  // Закрываем неиспользуемый slave FD

        // Читаем вывод от дочернего процесса и отправляем команды, если нужно
        char buf[256];
        ssize_t nread;
        while ((nread = read(master_fd, buf, sizeof(buf) - 1)) > 0) {
            buf[nread] = '\0';
            printf("Received: %s", buf);
        }

        close(master_fd);  // Закрываем master FD

        // Ожидаем завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Process exited with status %d\n", WEXITSTATUS(status));
        }
    }

    return 0;
}
