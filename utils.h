#ifndef UTILS_H
#define UTILS_H

// Funkcja do czyszczenia zasobów systemowych
void cleanup_resources();

// Funkcja obsługująca sygnał przerwania (Ctrl+C)
void signal_handler(int sig);

// Funkcja do inicjalizacji obsługi sygnałów
void setup_signal_handlers();

// Funkcja do walidacji danych wejściowych
int validate_input(int customers_per_register, int num_customers);

#endif // UTILS_H
