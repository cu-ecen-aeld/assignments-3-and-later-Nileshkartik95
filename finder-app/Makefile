.PHONY: all
all: writer

writer: writer_o
	gcc -o writer writer.o

CROSS_COMPILE: writer_aarch_o
	aarch64_be-none-linux-gnu-gcc -o writer writer.o

writer_o:
	gcc -c writer.c -o writer.o 

writer_aarch_o:
	aarch64_be-none-linux-gnu-gcc -c writer.c -o writer.o

.PHONY: clean
clean:
	rm -f *.o writer
