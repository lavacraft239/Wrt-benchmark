CC = gcc
CFLAGS = -lcurl -lpthread -Wall -O2

# Archivos fuente
SRC = wrt.c wrtHome.c

# Archivo objeto (compilado)
OBJ = $(SRC:.c=.o)

# Nombre del ejecutable
TARGET = wrt

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
