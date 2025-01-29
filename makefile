# Plik Makefile

# Kompilator
CC = gcc

# Flagi kompilatora
CFLAGS = -Wall -Wextra -std=c99 -g

# Pliki źródłowe
SRC = main.c manager.c customer.c firefighter.c

# Pliki nagłówkowe
HEADERS = manager.h customer.h firefighter.h

# Plik wynikowy
TARGET = supermarket

# Reguła główna
all: $(TARGET)

# Reguła budowy programu
$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Reguła czyszcząca pliki wynikowe
clean:
	rm -f $(TARGET)
	rm -f *.o

# Reguła testowania
test: $(TARGET)
	./$(TARGET)

# Domyślna reguła uruchamiania
run: $(TARGET)
	./$(TARGET)

# Reguła pomocy
help:
	@echo "Dostępne komendy:"
	@echo "  make all      - Kompiluje program"
	@echo "  make clean    - Usuwa pliki wynikowe"
	@echo "  make test     - Kompiluje i uruchamia program"
	@echo "  make run      - Uruchamia skompilowany program"
