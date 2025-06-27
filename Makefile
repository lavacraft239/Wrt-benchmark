all: wrt wrtHome

wrt: wrt.c
	$(CC) -o wrt wrt.c $(CFLAGS)

wrtHome: wrtHome.c
	$(CC) -o wrtHome wrtHome.c $(CFLAGS)

clean:
	rm -f wrt wrtHome
