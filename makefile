# Kompilator i opcje
CC = gcc
CFLAGS = -Wall -Wextra -pthread

# Pliki źródłowe
SOURCES = main.c manager.c customer.c fireman.c utils.c error.c
HEADERS = common.h utils.h

# Plik wykonywalny
TARGET = supermarket

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

fire:
	echo "FIRE" > /tmp/supermarket_fifo

.PHONY: all clean run fire
