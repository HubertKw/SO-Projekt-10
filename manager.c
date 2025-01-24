
#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stddef.h>
#include <errno.h>

// Funkcja blokująca semafor (wejście do sekcji krytycznej)
void sem_lock(int sem_id) {
    struct sembuf operation = {0, -1, 0};
    if (semop(sem_id, &operation, 1) == -1) {
        perror("Błąd blokowania semafora");
        exit(EXIT_FAILURE);
    }
}

// Funkcja odblokowująca semafor (wyjście z sekcji krytycznej)
void sem_unlock(int sem_id) {
    struct sembuf operation = {0, 1, 0};
    if (semop(sem_id, &operation, 1) == -1) {
        perror("Błąd odblokowywania semafora");
        exit(EXIT_FAILURE);
    }
}

// Funkcja inicjalizująca zarządzanie kasami
int initialize_manager(int shm_id, int sem_id, int initial_cashiers) {
    // Przyłączanie pamięci współdzielonej
    int *cashiers = (int *)shmat(shm_id, NULL, 0);
    if (cashiers == (void *)-1) {
        perror("Błąd przypisania pamięci współdzielonej w managerze");
        return -1;
    }

    sem_lock(sem_id); // Blokada dostępu do pamięci współdzielonej

    // Ustawienie stanu kas: początkowo otwarte są initial_cashiers kas
    for (int i = 0; i < MAX_CASHIERS; i++) {
        cashiers[i] = (i < initial_cashiers) ? 1 : 0;
    }

    sem_unlock(sem_id); // Odblokowanie dostępu do pamięci współdzielonej

    // Odłączenie pamięci współdzielonej
    if (shmdt(cashiers) == -1) {
        perror("Błąd odłączania pamięci współdzielonej w managerze");
        return -1;
    }

    return 0;
}
