# Имена исходных файлов
SOURCES=key.c term.c utf8.c msg.c ctrlstk.c iface.c kbd.c event.c mbuf.c history.c main.c
M4_HEAD=key_map.m4

OBJECTS=$(SOURCES:.c=.o)
M4_HEADERS=$(M4_HEAD:.m4=.h)
HEADERS=all_libs.h key_map.h key.h term.h utf8.h msg.h ctrlstk.h iface.h kbd.h event.h mbuf.h history.h

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

# Правило для тестирования
test: test.c
	gcc -o test test.c

# valgrind
valgrind:
	valgrind --leak-check=full ./$(TARGET)

# callgrind
callgrind:
	valgrind --tool=callgrind ./a.out

gdb:
	gdb $(TARGET)
#   # run
#   # bt

gpt:
	./gpt.pl main.c
	gedit output.c

etags:
	find . -name "*.[chCH]" -print | etags -

# Очистка
clean:
	rm -f $(TARGET) $(OBJECTS) $(M4_HEADERS)

.PHONY: all clean valgrind gdb
