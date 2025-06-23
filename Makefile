CC = gcc
CFLAGS = -Wall -O2
LIBS = -lcurl -lpthread
TARGET = wrt
SRC = wrt.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET) *.o wrt_results.csv
