#include "common.h"
#include "utils.h"

// Główna funkcja procesu strażaka
void fireman_process() {
    int shm_id, sem_id, fire_fd;
    char fifo_buffer[256];
    SharedMemory *shm;
    
    printf("STRAŻAK: Proces strażaka uruchomiony.\n");
    
    // Pobierz identyfikator pamięci dzielonej
    if ((shm_id = shmget(SHM_KEY, sizeof(SharedMemory), 0666)) == -1) {
        handle_error(ERROR_MEMORY, "Strażak nie może uzyskać dostępu do pamięci dzielonej");
        return;
    }
    
    // Dołącz pamięć dzieloną
    if ((shm = (SharedMemory *)shmat(shm_id, NULL, 0)) == (void *)-1) {
        handle_error(ERROR_MEMORY, "Strażak nie może przyłączyć pamięci dzielonej");
        return;
    }
    
    // Pobierz identyfikator semaforów
    if ((sem_id = semget(SEM_KEY, SEM_TOTAL, 0666)) == -1) {
        handle_error(ERROR_SEMAPHORE, "Strażak nie może uzyskać dostępu do semaforów");
        shmdt(shm);
        return;
    }
    
    // Utworzenie potoku nazwanego (FIFO) dla komunikacji z konsolą użytkownika
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            handle_error(ERROR_FIFO, "Strażak nie może utworzyć potoku nazwanego");
            shmdt(shm);
            return;
        }
    }
    
    printf("STRAŻAK: Nasłuchuję na komunikaty o pożarze poprzez FIFO: %s\n", FIFO_PATH);
    printf("STRAŻAK: Aby wywołać alarm pożarowy, użyj komendy: echo 'FIRE' > %s\n", FIFO_PATH);
    
    // Otwórz FIFO do odczytu
    if ((fire_fd = open(FIFO_PATH, O_RDONLY)) == -1) {
        handle_error(ERROR_FIFO, "Strażak nie może otworzyć potoku nazwanego do odczytu");
        shmdt(shm);
        unlink(FIFO_PATH);
        return;
    }
    
    // Główna pętla nasłuchiwania na komunikaty o pożarze
    while (1) {
        ssize_t bytes_read = read(fire_fd, fifo_buffer, sizeof(fifo_buffer) - 1);
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał, kontynuujemy
                continue;
            }
            handle_error(ERROR_FIFO, "Błąd odczytu z potoku nazwanego");
            break;
        } else if (bytes_read == 0) {
            // FIFO został zamknięty po stronie zapisu, otwórz go ponownie
            close(fire_fd);
            if ((fire_fd = open(FIFO_PATH, O_RDONLY)) == -1) {
                handle_error(ERROR_FIFO, "Strażak nie może ponownie otworzyć potoku nazwanego");
                break;
            }
            continue;
        }
        
        fifo_buffer[bytes_read] = '\0';
        
        // Sprawdź, czy otrzymano komendę alarmu pożarowego
        if (strstr(fifo_buffer, "FIRE") != NULL) {
            printf("STRAŻAK: Otrzymano zgłoszenie pożaru! Ogłaszam alarm i ewakuację!\n");
            
            // Ustaw flagę alarmu pożarowego w pamięci dzielonej
            struct sembuf sem_op;
            sem_op.sem_num = SEM_ACCESS;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            
            if (semop(sem_id, &sem_op, 1) == -1) {
                handle_error(ERROR_SEMAPHORE, "Strażak nie może zablokować semafora dostępu");
                continue;
            }
            
            shm->fire_alarm = 1;
            
            sem_op.sem_num = SEM_ACCESS;
            sem_op.sem_op = 1;
            sem_op.sem_flg = 0;
            
            if (semop(sem_id, &sem_op, 1) == -1) {
                handle_error(ERROR_SEMAPHORE, "Strażak nie może zwolnić semafora dostępu");
                continue;
            }
            
            // Wyślij sygnał do wszystkich procesów w grupie procesów
            if (kill(0, FIRE_SIGNAL) == -1) {
                handle_error(ERROR_SIGNAL, "Strażak nie może wysłać sygnału alarmu pożarowego");
            }
            
            printf("STRAŻAK: Alarm pożarowy został ogłoszony.\n");
            break;
        }
    }
    
    // Odłączenie pamięci dzielonej
    if (shmdt(shm) == -1) {
        handle_error(ERROR_MEMORY, "Strażak nie może odłączyć pamięci dzielonej");
    }
    
    // Zamknięcie potoku nazwanego
    close(fire_fd);
    
    printf("STRAŻAK: Proces strażaka zakończony.\n");
}
