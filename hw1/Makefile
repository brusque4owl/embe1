#CC		= gcc
CC		= arm-none-linux-gnueabi-gcc
CFLAGS	= -W -Wall
TARGET	= main

all : $(TARGET)

$(TARGET) : readkey.o sema.o main.o
	$(CC) $(CFLAGS) -static -o $(TARGET) readkey.o sema.o main.o

readkey.o : readkey.c
	$(CC) $(CFALGS) -static -c -o readkey.o readkey.c
sema.o : sema.c
	$(CC) $(CFLAGS) -static -c -o sema.o sema.c
main.o : main.c
	$(CC) $(CFLAGS) -static -c -o main.o main.c

clean :
	rm -rf *.o main
