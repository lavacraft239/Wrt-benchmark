wrt: wrt.o
    gcc -o wrt wrt.o -lcurl -lpthread -Wall -O2

wrtHome: wrtHome.o
    gcc -o wrtHome wrtHome.o -lcurl -lpthread -Wall -O2

wrt.o: wrt.c
    gcc -c wrt.c -o wrt.o

wrtHome.o: wrtHome.c
    gcc -c wrtHome.c -o wrtHome.o

clean:
    rm -f wrt wrtHome *.o
