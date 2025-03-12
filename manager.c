#include "common.h"
#include "utils.h"

// Funkcja obsługująca sygnał od strażaka (alarm pożarowy)
void handle_fire_alarm(int sig) {
    (void)sig; // Pomijamy niewykorzystany parametr
    int shm_id, msg_id;
    SharedMemory *shm;
    Message msg;
    
    // Pobierz identyfikator pamięci dzielonej
    if ((shm_id = shmget(SHM_KEY, sizeof(SharedMemory), 0666)) == -1) {
        handle_error(ERROR_MEMORY, "Nie można uzyskać dostępu do pamięci dzielonej w obsłudze sygnału");
        return;
    }
    
    // Dołącz pamięć dzieloną
    if ((shm = (SharedMemory *)shmat(shm_id, NULL, 0)) == (void *)-1) {
        handle_error(ERROR_MEMORY, "Nie można przyłączyć pamięci dzielonej w obsłudze sygnału");
        return;
    }
    
    // Pobierz identyfikator kolejki komunikatów
    if ((msg_id = msgget(MSG_KEY, 0666)) == -1) {
        handle_error(ERROR_MESSAGE_QUEUE, "Nie można uzyskać dostępu do kolejki komunikatów w obsłudze sygnału");
        shmdt(shm);
        return;
    }
    
    // Ustaw flagę alarmu pożarowego
    shm->fire_alarm = 1;
    
    // Odłącz pamięć dzieloną
    if (shmdt(shm) == -1) {
        handle_error(ERROR_MEMORY, "Nie można odłączyć pamięci dzielonej w obsłudze sygnału");
    }
    
    // Wyślij wiadomość o alarmie pożarowym
    msg.mtype = MSG_FIRE_ALARM;
    msg.action = 1;
    
    if (msgsnd(msg_id, &msg, sizeof(Message) - sizeof(long), 0) == -1) {
        handle_error(ERROR_MESSAGE_QUEUE, "Nie można wysłać wiadomości o alarmie pożarowym");
    }
    
    printf("KIEROWNIK: Alarm pożarowy! Ewakuacja klientów i zamykanie kas.\n");
}

// Główna funkcja kierownika
void manager_process(int customers_per_register) {
    int shm_id, sem_id, msg_id;
    SharedMemory *shm;
    Message msg;
    struct sembuf sem_op;
    
    // Rejestracja obsługi sygnału alarmu pożarowego
    if (signal(FIRE_SIGNAL, handle_fire_alarm) == SIG_ERR) {
        handle_error(ERROR_SIGNAL, "Nie można zarejestrować obsługi sygnału alarmu pożarowego");
        return;
    }
    
    // Pobierz identyfikator pamięci dzielonej
    if ((shm_id = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666)) == -1) {
        handle_error(ERROR_MEMORY, "Nie można utworzyć segmentu pamięci dzielonej");
        return;
    }
    
    // Dołącz pamięć dzieloną
    if ((shm = (SharedMemory *)shmat(shm_id, NULL, 0)) == (void *)-1) {
        handle_error(ERROR_MEMORY, "Nie można przyłączyć pamięci dzielonej");
        return;
    }
    
    // Inicjalizacja pamięci dzielonej
    shm->active_customers = 0;
    shm->active_registers = MIN_REGISTERS;
    shm->customers_per_register = customers_per_register;
    shm->fire_alarm = 0;
    
    for (int i = 0; i < MAX_REGISTERS; i++) {
        shm->customers_in_queue[i] = 0;
        shm->register_status[i] = (i < MIN_REGISTERS) ? 1 : 0;
    }
    
    // Pobierz identyfikator semaforów
    if ((sem_id = semget(SEM_KEY, SEM_TOTAL, IPC_CREAT | 0666)) == -1) {
        handle_error(ERROR_SEMAPHORE, "Nie można utworzyć zestawu semaforów");
        shmdt(shm);
        return;
    }
    
    // Inicjalizacja semaforów
    if (semctl(sem_id, SEM_ACCESS, SETVAL, 1) == -1 ||
        semctl(sem_id, SEM_MANAGER, SETVAL, 1) == -1 ||
        semctl(sem_id, SEM_CUSTOMER, SETVAL, 1) == -1) {
        handle_error(ERROR_SEMAPHORE, "Nie można zainicjalizować semaforów");
        shmdt(shm);
        return;
    }
    
    // Pobierz identyfikator kolejki komunikatów
    if ((msg_id = msgget(MSG_KEY, IPC_CREAT | 0666)) == -1) {
        handle_error(ERROR_MESSAGE_QUEUE, "Nie można utworzyć kolejki komunikatów");
        shmdt(shm);
        return;
    }
    
    printf("KIEROWNIK: Inicjalizacja supermarketu zakończona.\n");
    printf("KIEROWNIK: Wartość K (klienci na kasę): %d\n", customers_per_register);
    printf("KIEROWNIK: Minimalnie czynne kasy: %d\n", MIN_REGISTERS);
    
    // Główna pętla obsługi supermarketu
    while (1) {
        // Odbiór wiadomości
        if (msgrcv(msg_id, &msg, sizeof(Message) - sizeof(long), 0, 0) == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał, kontynuujemy
                continue;
            }
            handle_error(ERROR_MESSAGE_QUEUE, "Błąd podczas odbierania wiadomości");
            break;
        }
        
        // Operacja na semaforze dostępu
        sem_op.sem_num = SEM_ACCESS;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        
        if (semop(sem_id, &sem_op, 1) == -1) {
            handle_error(ERROR_SEMAPHORE, "Nie można zablokować semafora dostępu");
            continue;
        }
        
        // Obsługa wiadomości w zależności od typu
        switch (msg.mtype) {
            case MSG_NEW_CUSTOMER:
                // Nowy klient wchodzi do supermarketu
                shm->active_customers++;
                printf("KIEROWNIK: Nowy klient %d w supermarkecie. Aktualnie klientów: %d\n", 
                       msg.customer_id, shm->active_customers);
                
                // Sprawdź, czy potrzeba otworzyć nową kasę
                if (shm->active_customers > shm->customers_per_register * shm->active_registers &&
                    shm->active_registers < MAX_REGISTERS) {
                    // Znajdź i otwórz zamkniętą kasę
                    for (int i = 0; i < MAX_REGISTERS; i++) {
                        if (shm->register_status[i] == 0) {
                            shm->register_status[i] = 1;
                            shm->active_registers++;
                            
                            printf("KIEROWNIK: Otwieram kasę %d. Aktualnie czynne kasy: %d\n", 
                                   i, shm->active_registers);
                            
                            // Wyślij wiadomość o otwarciu nowej kasy
                            Message open_msg;
                            open_msg.mtype = MSG_REGISTER_OPEN;
                            open_msg.register_id = i;
                            
                            if (msgsnd(msg_id, &open_msg, sizeof(Message) - sizeof(long), 0) == -1) {
                                handle_error(ERROR_MESSAGE_QUEUE, "Nie można wysłać wiadomości o otwarciu kasy");
                            }
                            
                            break;
                        }
                    }
                }
                break;
                
            case MSG_CUSTOMER_LEAVES:
                // Klient opuszcza supermarket
                if (shm->active_customers > 0) {
                    shm->active_customers--;
                }
                
                printf("KIEROWNIK: Klient %d opuścił supermarket. Aktualnie klientów: %d\n", 
                       msg.customer_id, shm->active_customers);
                
                // Sprawdź, czy można zamknąć kasę
                if (shm->active_customers < shm->customers_per_register * (shm->active_registers - 1) &&
                    shm->active_registers > MIN_REGISTERS) {
                    // Znajdź kasę z najmniejszą kolejką do zamknięcia
                    int min_queue = -1;
                    int min_register = -1;
                    
                    for (int i = 0; i < MAX_REGISTERS; i++) {
                        if (shm->register_status[i] == 1) {
                            if (min_queue == -1 || shm->customers_in_queue[i] < min_queue) {
                                min_queue = shm->customers_in_queue[i];
                                min_register = i;
                            }
                        }
                    }
                    
                    if (min_register != -1) {
                        printf("KIEROWNIK: Planuję zamknięcie kasy %d. Klientów w kolejce: %d\n", 
                               min_register, shm->customers_in_queue[min_register]);
                        
                        // Jeśli kolejka jest pusta, zamykamy natychmiast
                        if (shm->customers_in_queue[min_register] == 0) {
                            shm->register_status[min_register] = 0;
                            shm->active_registers--;
                            
                            printf("KIEROWNIK: Zamykam kasę %d. Aktualnie czynne kasy: %d\n", 
                                   min_register, shm->active_registers);
                            
                            // Wyślij wiadomość o zamknięciu kasy
                            Message close_msg;
                            close_msg.mtype = MSG_REGISTER_CLOSE;
                            close_msg.register_id = min_register;
                            
                            if (msgsnd(msg_id, &close_msg, sizeof(Message) - sizeof(long), 0) == -1) {
                                handle_error(ERROR_MESSAGE_QUEUE, "Nie można wysłać wiadomości o zamknięciu kasy");
                            }
                        }
                    }
                }
                break;
                
            case MSG_FIRE_ALARM:
                // Obsługa alarmu pożarowego
                if (msg.action == 1) {
                    printf("KIEROWNIK: Rozpoczynam procedurę ewakuacji.\n");
                    
                    // Zamknij wszystkie kasy po ewakuacji klientów
                    if (shm->active_customers == 0) {
                        for (int i = 0; i < MAX_REGISTERS; i++) {
                            if (shm->register_status[i] == 1) {
                                shm->register_status[i] = 0;
                            }
                        }
                        shm->active_registers = 0;
                        printf("KIEROWNIK: Wszystkie kasy zostały zamknięte po ewakuacji.\n");
                    }
                }
                break;
        }
        
        // Zwolnij semafor dostępu
        sem_op.sem_num = SEM_ACCESS;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        
        if (semop(sem_id, &sem_op, 1) == -1) {
            handle_error(ERROR_SEMAPHORE, "Nie można zwolnić semafora dostępu");
        }
        
        // Jeśli alarm pożarowy i wszyscy klienci opuścili supermarket, kończymy działanie
        if (shm->fire_alarm && shm->active_customers == 0) {
            printf("KIEROWNIK: Ewakuacja zakończona, kończę pracę.\n");
            break;
        }
    }
    
    // Odłączenie pamięci dzielonej
    if (shmdt(shm) == -1) {
        handle_error(ERROR_MEMORY, "Nie można odłączyć pamięci dzielonej");
    }
    
    printf("KIEROWNIK: Proces kierownika zakończony.\n");
}
