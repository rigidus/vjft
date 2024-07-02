// loader.c
#include <stdio.h>
#include <stdlib.h>
#include <ubpf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    // Инициализация виртуальной машины uBPF
    struct ubpf_vm *vm = ubpf_create();
    if (vm == NULL) {
        fprintf(stderr, "Failed to create uBPF VM\n");
        return 1;
    }

    // Загрузка uBPF программы из файла
    int fd = open("minimal_bpf.o", O_RDONLY);
    if (fd == -1) {
        perror("open");
        ubpf_destroy(vm);
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        ubpf_destroy(vm);
        return 1;
    }

    void *code = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        close(fd);
        ubpf_destroy(vm);
        return 1;
    }

    char *errmsg;
    /* int rv = ubpf_load_elf(vm, code, st.st_size, &errmsg); */
    int rv = ubpf_load_elf_ex(vm, code, st.st_size, "bpf_prog1", &errmsg);
    if (rv < 0) {
        fprintf(stderr, "Failed to load code: %s\n", errmsg);
        free(errmsg);
        munmap(code, st.st_size);
        close(fd);
        ubpf_destroy(vm);
        return 1;
    }

    munmap(code, st.st_size);
    close(fd);

    // Выполнение uBPF программы
    uint64_t ret_val;
    rv = ubpf_exec(vm, NULL, 0, &ret_val);
    if (rv < 0) {
        fprintf(stderr, "Failed to execute program\n");
        ubpf_destroy(vm);
        return 1;
    }

    printf("Program returned: %lu\n", ret_val);

    // Освобождение ресурсов
    ubpf_destroy(vm);
    return 0;
}
