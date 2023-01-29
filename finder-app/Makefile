CROSS_COMPILED ?= $(CROSS_COMPILE)gcc				#by default compiler is gcc is specified aarch64-none-linux-gnu gcc will be appended at the end
COMPILER_FLAGS=-Wall -Werror -Wextra 				#variable to hold the various flag for cross compiling

.PHONY: all
all: writer

writer : writer_o
	$(CROSS_COMPILED) $(COMPILER_FLAGS) -o writer writer.o	#linking the object file to the executable , with warning error and Wextra flag

writer_o:
	$(CROSS_COMPILED) $(COMPILER_FLAGS) -c writer.c -o writer.o 	#creating the object file for writer.c and warning erorr and Wextra flag being set
	
.PHONY: clean
clean:
	rm -f *.o writer						#cleaning all the associated object file pertaining to writer executable
