# Имена исходных файлов
SOURCES=term.c
M4_HEAD=term.m4

OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(M4_HEAD:.m4=.h)

# Имя целевого исполняемого файла
TARGET=term

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
$(OBJECTS): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -c $<

$(HEADERS): $(M4_HEAD)
	m4 $< > $@

# valgrind
valgrind:
	valgrind --leak-check=full ./($TARGET)

gdb:
	gdb $(TARGET)
# 	# run
# 	# bt


# Очистка
clean:
	rm -f $(TARGET) $(OBJECTS) $(HEADERS)


.PHONY: all clean
