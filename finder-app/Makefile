.PHONY: all
all: writer

writer : writer_o
	gcc -Wall -Werror -Wextra -o writer writer.o					#linking the object file to the executable for GCC Compiler, with warning error and Wextra flag

CROSS_COMPILE: writer_aarch_o
	aarch64-none-linux-gnu-gcc -Wall -Werror -Wextra -o writer writer.o		#linking the object file to the executable for cross compiler Compiler, with warning error and Wextra flag

writer_o:
	gcc -Wall -Werror -Wextra -c writer.c -o writer.o 				#creating the object file for writer.c with native compiler and warning erorr and Wextra flag being set
	
writer_aarch_o:
	aarch64-none-linux-gnu-gcc -Wall -Werror -Wextra -c writer.c -o writer.o	#creating the object file for writer.c with cross compiler and warning erorr and Wextra flag being set

.PHONY: clean
clean:
	rm -f *.o writer								#cleaning all the associated object file pertaining to writer executable
