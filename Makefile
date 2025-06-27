# Makefile para compilar wrt.c y wrtHome.c

CC = gcc
CFLAGS = -Wall -O2 -lcurl -lpthread

all: wrt

wrt: wrt.o wrtHome.o
	$(CC) -o wrt wrt.o wrtHome.o $(CFLAGS)

wrt.o: wrt.c
	$(CC) -c wrt.c -o wrt.o

wrtHome.o: wrtHome.c
	$(CC) -c wrtHome.c -o wrtHome.o

clean:
	rm -f wrt wrt.o wrtHome.o
