#include "customer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

// Funkcja uruchamiająca procesy klientów
int start_customers(int shm_id, int sem_id) {
    (void)shm_id;
    (void)sem_id;

    pid_t pid = fork();
    if (pid == -1) {
        perror("Błąd tworzenia procesu klienta");
        return -1;
    }

    if (pid == 0) {
        // Proces klienta: symulacja wchodzenia do supermarketu
        while (1) {
            sleep(rand() % 5 + 1); // Losowy czas przyjścia klienta
            printf("Nowy klient wchodzi do supermarketu.\n");
        }
        exit(0);
    }

    return 0;
}
