# Имена исходных файлов
SOURCES=term.c msg.c ctrlstk.c main.c
M4_HEAD=key_map.m4

OBJECTS=$(SOURCES:.c=.o)
M4_HEADERS=$(M4_HEAD:.m4=.h)
HEADERS=key_map.h term.h msg.h ctrlstk.h

# Имя целевого исполняемого файла
TARGET=main

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
