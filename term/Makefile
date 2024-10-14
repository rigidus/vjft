# Имена исходных файлов
SOURCES=key.c term.c utf8.c msg.c ctrlstk.c iface.c kbd.c event.c mbuf.c history.c client.c crypt.c main.c
M4_HEAD=key_map.m4

OBJECTS=$(SOURCES:.c=.o)
M4_HEADERS=$(M4_HEAD:.m4=.h)
HEADERS=defs.h all_libs.h key_map.h key.h term.h utf8.h msg.h ctrlstk.h iface.h kbd.h event.h mbuf.h history.h client.h crypt.h

# Библиотеки для линковки
LIBS=-lssl -lcrypto

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
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

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

# Правило для тестирования encipher/decipher
ende: all_libs.h defs.h crypt.h crypt.c ende.c
	gcc -o $@ -DDBG_MSG=1 $^ $(LIBS)

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


srv:
	../srv/chat_server 8888

alice:
	./a.out 127.0.0.1 8888 alice_private_key.pem qwe123 bob_public_key.pem carol_public_key.pem

bob:
	./a.out 127.0.0.1 8888 bob_private_key.pem qwe123 alice_public_key.pem carol_public_key.pem

carol:
	./a.out 127.0.0.1 8888 carol_private_key.pem qwe123 bob_public_key.pem alice_public_key.pem


gen_alice:
	openssl genpkey -algorithm RSA -out alice_private_key.pem -pkeyopt rsa_keygen_bits:4096 -aes256 -pass pass:qwe123
	openssl rsa -pubout -in alice_private_key.pem -out alice_public_key.pem -passin pass:qwe123

gen_bob:
	openssl genpkey -algorithm RSA -out bob_private_key.pem -pkeyopt rsa_keygen_bits:4096 -aes256 -pass pass:qwe123
	openssl rsa -pubout -in bob_private_key.pem -out bob_public_key.pem -passin pass:qwe123

gen_carol:
	openssl genpkey -algorithm RSA -out carol_private_key.pem -pkeyopt rsa_keygen_bits:4096 -aes256 -pass pass:qwe123
	openssl rsa -pubout -in carol_private_key.pem -out carol_public_key.pem -passin pass:qwe123


# Очистка
clean:
	rm -f $(TARGET) $(OBJECTS) $(M4_HEADERS)

.PHONY: all clean valgrind gdb
