# Имена исходных файлов
SOURCES=key.c term.c utf8.c msg.c ctrlstk.c cmd.c iface.c kbd.c main.c
M4_HEAD=key_map.m4

OBJECTS=$(SOURCES:.c=.o)
M4_HEADERS=$(M4_HEAD:.m4=.h)
HEADERS=all_libs.h key_map.h key.h term.h utf8.h msg.h ctrlstk.h cmd.h iface.h kbd.h

# Имя целевого исполняемого файла
TARGET=a.out

# Компилятор
CC=gcc

# Флаги для компилятора
CFLAGS=-Wall -g

# Правило по умолчанию
all: $(TARGET)

# Цель
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Правило для сборки объектных файлов
$(OBJECTS): %.o : %.c $(HEADERS) $(M4_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Правило для генерации заголовков из m4
key_map.h: $(M4_HEAD)
	m4 $< > $@

# $(HEADERS): $(M4_HEAD)
# 	m4 $< > $@

# valgrind
valgrind:
	valgrind --leak-check=full ./$(TARGET)

gdb:
	gdb $(TARGET)
#   # run
#   # bt

# Очистка
clean:
	rm -f $(TARGET) $(OBJECTS) $(M4_HEADERS)

.PHONY: all clean valgrind gdb
