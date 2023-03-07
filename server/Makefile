CC?=$(CROSS_COMPILE)gcc				#by default compiler is gcc is specified aarch64-none-linux-gnu gcc will be appended at the end
CFLAGS?=-g -Wall -Werror -Wextra 				#variable to hold the various flag for cross compiling
LDFLAGS?=-pthread -lrt

.PHONY: all
all: aesdsocket

aesdsocket : aesdsocket_o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o  $(LDFLAGS) 	#linking the object file to the executable , with warning error and Wextra flag

aesdsocket_o:
	$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o	#creating the object file for writer.c and warning erorr and Wextra flag being set
	
.PHONY: clean
clean:
	rm -f *.o aesdsocket						#cleaning all the associated object file pertaining to writer executable
