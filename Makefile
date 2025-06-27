CC = cc
CFLAGS = -Wall -O2
LDFLAGS = -lcurl -lpthread

all: wrt

wrt: wrt.o
	$(CC) -o wrt wrt.o $(LDFLAGS)

wrt.o: wrt.c
	$(CC) $(CFLAGS) -c wrt.c

clean:
	rm -f wrt wrt.o
