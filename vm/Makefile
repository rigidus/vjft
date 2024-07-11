BPF_CLANG=clang
BPF_CFLAGS=-O0 -target bpf -I/usr/include/x86_64-linux-gnu
TARGET = minimal_bpf.o


all: $(TARGET)

minimal_bpf.o: minimal_bpf.c
	$(BPF_CLANG) $(BPF_CFLAGS) -c minimal_bpf.c -o $(TARGET)

headers:
	llvm-readelf -x $(TARGET)

sections:
	llvm-objdump -h $(TARGET)

symbols:
	llvm-objdump -t $(TARGET)

disasm:
	llvm-objdump -d --line-numbers $(TARGET)

dis_src:
	llvm-objdump -d --line-numbers --source $(TARGET)

relo:
	llvm-objdump -r $(TARGET)

rub:
	gcc run_ubpf.c -lubpf -o run_ubpf

code:
	# "prog" section number is 3
	readelf -x 3 $(TARGET)

objdump:
	llvm-objdump -S $(TARGET)

loader: loader.c
	gcc loader.c -I../../ubpf/vm/inc -I../../ubpf/build/vm/ -L../../ubpf/build/lib -lubpf  -o loader

load:
	./loader

clean:
	rm -f *.o
