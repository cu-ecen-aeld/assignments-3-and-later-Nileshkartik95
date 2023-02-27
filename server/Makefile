CC=$(CROSS_COMPILE)gcc				#by default compiler is gcc is specified aarch64-none-linux-gnu gcc will be appended at the end
COMPILER_FLAGS=-Wall -Werror -Wextra 				#variable to hold the various flag for cross compiling

.PHONY: all
all: aesdsocket

aesdsocket : aesdsocket_o
	$(CC) $(COMPILER_FLAGS) -o aesdsocket aesdsocket.o	#linking the object file to the executable , with warning error and Wextra flag

aesdsocket_o:
	$(CC) $(COMPILER_FLAGS) -c aesdsocket.c -o aesdsocket.o 	#creating the object file for writer.c and warning erorr and Wextra flag being set
	
.PHONY: clean
clean:
	rm -f *.o aesdsocket						#cleaning all the associated object file pertaining to writer executable
