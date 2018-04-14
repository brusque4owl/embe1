#CC		= gcc
CC		= arm-none-linux-gnueabi-gcc
CFLAGS	= -W -Wall
TARGET	= main

all : $(TARGET)

$(TARGET) : mode5.o mode4.o mode3.o mode2.o mode1.o sema.o main.o
	$(CC) $(CFLAGS) -static -o $(TARGET) mode5.o mode4.o mode3.o mode2.o mode1.o sema.o main.o

mode5.o : mode5.c
	$(CC) $(CFLAGS) -static -c -o mode5.o mode5.c
mode4.o : mode4.c
	$(CC) $(CFLAGS) -static -c -o mode4.o mode4.c
mode3.o : mode3.c
	$(CC) $(CFLAGS) -static -c -o mode3.o mode3.c
mode2.o : mode2.c
	$(CC) $(CFLAGS) -static -c -o mode2.o mode2.c
mode1.o : mode1.c
	$(CC) $(CFLAGS) -static -c -o mode1.o mode1.c
sema.o : sema.c
	$(CC) $(CFLAGS) -static -c -o sema.o sema.c
main.o : main.c
	$(CC) $(CFLAGS) -static -c -o main.o main.c

clean :
	rm -rf *.o main
