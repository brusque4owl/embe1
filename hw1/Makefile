#CC		= gcc
CC		= arm-none-linux-gnueabi-gcc
CFLAGS	= -W -Wall
TARGET	= main

all : $(TARGET)

$(TARGET) : mode1.o sema.o main.o
	$(CC) $(CFLAGS) -static -o $(TARGET) mode1.o sema.o main.o

mode1.o : mode1.c
	$(CC) $(CFLAGS) -static -c -o mode1.o mode1.c
sema.o : sema.c
	$(CC) $(CFLAGS) -static -c -o sema.o sema.c
main.o : main.c
	$(CC) $(CFLAGS) -static -c -o main.o main.c

clean :
	rm -rf *.o main
