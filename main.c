#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include "manager.h"
#include "customer.h"
#include "firefighter.h"

#define K 5 // Minimalna liczba klientów na kasę
#define MAX_CASHIERS 10 // Maksymalna liczba kas w supermarkecie
#define MIN_CASHIERS 2 // Minimalna liczba otwartych kas

// Globalne zmienne
int shm_id; // ID pamięci współdzielonej
int sem_id; // ID semaforów

// Funkcja czyszcząca zasoby systemowe
void cleanup() {
    // Usuwanie pamięci współdzielonej
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania pamięci współdzielonej");
    }
    // Usuwanie semaforów
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semaforów");
    }
}

// Obsługa sygnałów, np. SIGINT do zakończenia programu
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nZamykanie supermarketu...\n");
        cleanup();
        exit(0);
    }
}

int main() {
    // Rejestracja obsługi sygnałów
    signal(SIGINT, signal_handler);

    // Tworzenie pamięci współdzielonej na przechowywanie stanu kas
    shm_id = shmget(IPC_PRIVATE, sizeof(int) * MAX_CASHIERS, IPC_CREAT | 0660);
    if (shm_id == -1) {
        perror("Błąd tworzenia pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    // Tworzenie zestawu semaforów do synchronizacji procesów
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0660);
    if (sem_id == -1) {
        perror("Błąd tworzenia semaforów");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja wartości semafora na 1 (dostęp do sekcji krytycznej)
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("Błąd inicjalizacji semafora");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja modułu zarządzania kasami
    if (initialize_manager(shm_id, sem_id, MIN_CASHIERS) == -1) {
        perror("Błąd inicjalizacji managera");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Uruchamianie symulacji klientów
    if (start_customers(shm_id, sem_id) == -1) {
        perror("Błąd uruchamiania klientów");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Uruchamianie strażaka odpowiedzialnego za alarm pożarowy
    if (start_firefighter(shm_id, sem_id) == -1) {
        perror("Błąd uruchamiania strażaka");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Główna pętla programu (symulacja działania supermarketu)
    while (1) {
        sleep(1); // Wykonywanie operacji co sekundę
    }

    return 0;
}
