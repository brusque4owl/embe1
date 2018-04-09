#CC		= arm-none-linux-gnueabi-gcc
CC		= gcc
CFLAGS	= -W -Wall
TARGET	= main

all : $(TARGET)

$(TARGET) : sema.o main.o
	$(CC) $(CFLAGS) -static -o $(TARGET) sema.o main.o

sema.o : sema.c
	$(CC) $(CFLAGS) -static -c -o sema.o sema.c
main.o : main.c
	$(CC) $(CFLAGS) -static -c -o main.o main.c

clean :
	rm -rf *.o main
