#include "common.h"
#include "utils.h"

// Funkcja do czyszczenia zasobów systemowych
void cleanup_resources() {
    int shm_id, sem_id, msg_id;
    
    // Usuń segment pamięci dzielonej
    if ((shm_id = shmget(SHM_KEY, sizeof(SharedMemory), 0666)) != -1) {
        if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
            perror("Błąd podczas usuwania segmentu pamięci dzielonej");
        } else {
            printf("Segment pamięci dzielonej został usunięty.\n");
        }
    }
    
    // Usuń zestaw semaforów
    if ((sem_id = semget(SEM_KEY, SEM_TOTAL, 0666)) != -1) {
        if (semctl(sem_id, 0, IPC_RMID) == -1) {
            perror("Błąd podczas usuwania zestawu semaforów");
        } else {
            printf("Zestaw semaforów został usunięty.\n");
        }
    }
    
    // Usuń kolejkę komunikatów
    if ((msg_id = msgget(MSG_KEY, 0666)) != -1) {
        if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
            perror("Błąd podczas usuwania kolejki komunikatów");
        } else {
            printf("Kolejka komunikatów została usunięta.\n");
        }
    }
    
    // Usuń potok nazwany
    if (unlink(FIFO_PATH) == -1) {
        if (errno != ENOENT) { // Ignoruj błąd, jeśli plik nie istnieje
            perror("Błąd podczas usuwania potoku nazwanego");
        } else {
            printf("Potok nazwany został usunięty.\n");
        }
    }
}

// Funkcja obsługująca sygnał przerwania (Ctrl+C)
void signal_handler(int sig) {
    printf("\nOtrzymano sygnał przerwania (%d). Zamykanie supermarketu i czyszczenie zasobów...\n", sig);
    cleanup_resources();
    exit(0);
}

// Funkcja do inicjalizacji obsługi sygnałów
void setup_signal_handlers() {
    struct sigaction sa;
    
    // Konfiguracja obsługi sygnału przerwania (SIGINT)
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_error(ERROR_SIGNAL, "Nie można zarejestrować obsługi sygnału przerwania");
    }
    
    // Ignorowanie sygnału zerwania potoku (SIGPIPE)
    signal(SIGPIPE, SIG_IGN);
}

// Funkcja do walidacji danych wejściowych
int validate_input(int customers_per_register, int num_customers) {
    // Sprawdź, czy wartości są w dozwolonym zakresie
    if (customers_per_register <= 0 || customers_per_register > 50) {
        return 0; // Nieprawidłowa wartość
    }
    
    if (num_customers <= 0 || num_customers > MAX_CUSTOMERS) {
        return 0; // Nieprawidłowa wartość
    }
    
    return 1; // Wartości prawidłowe
}
