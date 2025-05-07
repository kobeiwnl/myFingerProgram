# Makefile per il progetto myFinger

CC = gcc
CFLAGS = -Wall -g

all: myFinger

myFinger: myFinger.o lib.o
	$(CC) -o myFinger myFinger.o lib.o

myFinger.o: myFinger.c lib.h
	$(CC) $(CFLAGS) -c myFinger.c

lib.o: lib.c lib.h
	$(CC) $(CFLAGS) -c lib.c

clean:
	rm -f *.o myFinger
