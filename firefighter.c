#include "firefighter.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

// Funkcja uruchamiająca strażaka (proces odpowiedzialny za alarm pożarowy)
int start_firefighter(int shm_id, int sem_id) {
    (void)shm_id; // Nie używane w tej implementacji
    (void)sem_id; // Nie używane w tej implementacji

    pid_t pid = fork();
    if (pid == -1) {
        perror("Błąd tworzenia procesu strażaka");
        return -1;
    }

    if (pid == 0) {
        // Proces strażaka: oczekiwanie na wygenerowanie alarmu pożarowego
        while (1) {
            sleep(rand() % 10 + 10); // Losowy czas do alarmu (10-20 sekund)
            printf("Strażak: Wykryto pożar! Generowanie sygnału zamknięcia supermarketu.\n");
            kill(getppid(), SIGINT); // Wysyłanie sygnału SIGINT do głównego procesu
        }
        exit(0);
    }

    return 0;
}
