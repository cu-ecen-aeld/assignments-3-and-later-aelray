ifdef CROSS_COMPILE
	PREFIX := aarch64-none-linux-gnu-
endif
CC := $(PREFIX)gcc

all: writer
	
writer.o:
	$(CC) -c writer.c -o writer.o

writer: writer.o
	$(CC) writer.o -o writer

clean:
	rm -f *.o writer