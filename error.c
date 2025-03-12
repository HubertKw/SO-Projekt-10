#include "common.h"

// Funkcja do obsługi błędów
void handle_error(int error_code, const char* message) {
    fprintf(stderr, "Błąd: %s\n", message);
    perror("Szczegóły błędu");
    
    // Wykonaj dodatkowe czynności w zależności od kodu błędu
    switch (error_code) {
        case ERROR_INIT:
            fprintf(stderr, "Błąd inicjalizacji programu\n");
            break;
        case ERROR_FORK:
            fprintf(stderr, "Błąd tworzenia procesu potomnego\n");
            break;
        case ERROR_MEMORY:
            fprintf(stderr, "Błąd alokacji pamięci dzielonej\n");
            break;
        case ERROR_SEMAPHORE:
            fprintf(stderr, "Błąd operacji na semaforze\n");
            break;
        case ERROR_MESSAGE_QUEUE:
            fprintf(stderr, "Błąd operacji na kolejce komunikatów\n");
            break;
        case ERROR_SIGNAL:
            fprintf(stderr, "Błąd obsługi sygnału\n");
            break;
        case ERROR_FIFO:
            fprintf(stderr, "Błąd operacji na potoku nazwanym (FIFO)\n");
            break;
        case ERROR_THREAD:
            fprintf(stderr, "Błąd operacji na wątku\n");
            break;
        default:
            fprintf(stderr, "Nieznany kod błędu\n");
    }
    
    // W zależności od powagi błędu możemy zakończyć program
    if (error_code <= ERROR_FORK) {
        fprintf(stderr, "Krytyczny błąd, kończenie programu\n");
        exit(EXIT_FAILURE);
    }
}
