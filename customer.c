#include "common.h"
#include "utils.h"

// Struktura danych klienta dla wątku
typedef struct {
    int id;
    int shopping_time;
    int shm_id;
    int sem_id;
    int msg_id;
} CustomerData;

// Funkcja obsługująca sygnał alarmu pożarowego dla klienta
void customer_handle_fire(int sig) {
    (void)sig; // Pomijamy niewykorzystany parametr
    printf("KLIENT: Otrzymano sygnał alarmu pożarowego! Natychmiastowa ewakuacja.\n");
    // Sygnał zostanie obsłużony w głównej pętli klienta
}

// Funkcja wątku klienta
void* customer_thread(void* arg) {
    CustomerData* data = (CustomerData*)arg;
    SharedMemory* shm;
    Message msg;
    struct sembuf sem_op;
    int register_id = -1;
    int min_queue = -1;
    
    // Konfiguracja obsługi sygnału alarmu pożarowego
    if (signal(FIRE_SIGNAL, customer_handle_fire) == SIG_ERR) {
        handle_error(ERROR_SIGNAL, "Nie można zarejestrować obsługi sygnału alarmu pożarowego dla klienta");
        free(data);
        pthread_exit(NULL);
    }
    
    // Przyłączenie do pamięci dzielonej
    if ((shm = (SharedMemory*)shmat(data->shm_id, NULL, 0)) == (void*)-1) {
        handle_error(ERROR_MEMORY, "Klient nie może przyłączyć pamięci dzielonej");
        free(data);
        pthread_exit(NULL);
    }
    
    // Wysłanie wiadomości o wejściu klienta do supermarketu
    msg.mtype = MSG_NEW_CUSTOMER;
    msg.customer_id = data->id;
    
    if (msgsnd(data->msg_id, &msg, sizeof(Message) - sizeof(long), 0) == -1) {
        handle_error(ERROR_MESSAGE_QUEUE, "Nie można wysłać wiadomości o nowym kliencie");
        shmdt(shm);
        free(data);
        pthread_exit(NULL);
    }
    
    printf("KLIENT %d: Wszedłem do supermarketu. Zakupy potrwają %d sekund.\n", 
           data->id, data->shopping_time);
    
    // Symulacja robienia zakupów
    int time_left = data->shopping_time;
    int evacuated = 0;
    
    while (time_left > 0) {
        sleep(1);
        time_left--;
        
        // Sprawdź, czy jest alarm pożarowy
        sem_op.sem_num = SEM_ACCESS;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        
        if (semop(data->sem_id, &sem_op, 1) == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał, kontynuujemy
                continue;
            }
            handle_error(ERROR_SEMAPHORE, "Klient nie może zablokować semafora dostępu");
            break;
        }
        
        int fire_alarm = shm->fire_alarm;
        
        sem_op.sem_num = SEM_ACCESS;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        
        if (semop(data->sem_id, &sem_op, 1) == -1) {
            handle_error(ERROR_SEMAPHORE, "Klient nie może zwolnić semafora dostępu");
            break;
        }
        
        if (fire_alarm) {
            printf("KLIENT %d: Alarm pożarowy! Przerywam zakupy i uciekam!\n", data->id);
            evacuated = 1;
            break;
        }
    }
    
    if (!evacuated) {
        printf("KLIENT %d: Zakończyłem zakupy. Szukam kasy z najkrótszą kolejką.\n", data->id);
        
        // Znajdź kasę z najkrótszą kolejką
        sem_op.sem_num = SEM_ACCESS;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        
        if (semop(data->sem_id, &sem_op, 1) == -1) {
            handle_error(ERROR_SEMAPHORE, "Klient nie może zablokować semafora dostępu");
            shmdt(shm);
            free(data);
            pthread_exit(NULL);
        }
        
        for (int i = 0; i < MAX_REGISTERS; i++) {
            if (shm->register_status[i] == 1) {
                if (min_queue == -1 || shm->customers_in_queue[i] < min_queue) {
                    min_queue = shm->customers_in_queue[i];
                    register_id = i;
                }
            }
        }
        
        if (register_id != -1) {
            shm->customers_in_queue[register_id]++;
            printf("KLIENT %d: Wybrałem kasę %d. W kolejce przede mną: %d osób.\n", 
                   data->id, register_id, min_queue);
        } else {
            printf("KLIENT %d: Nie znalazłem żadnej otwartej kasy!\n", data->id);
        }
        
        sem_op.sem_num = SEM_ACCESS;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        
        if (semop(data->sem_id, &sem_op, 1) == -1) {
            handle_error(ERROR_SEMAPHORE, "Klient nie może zwolnić semafora dostępu");
            shmdt(shm);
            free(data);
            pthread_exit(NULL);
        }
        
        if (register_id != -1) {
            // Symulacja czekania w kolejce (1 sekunda na osobę)
            sleep(min_queue);
            
            // Symulacja obsługi przy kasie (2-5 sekund)
            int service_time = (rand() % 4) + 2;
            printf("KLIENT %d: Jestem obsługiwany przy kasie %d. Obsługa potrwa %d sekund.\n", 
                   data->id, register_id, service_time);
            sleep(service_time);
            
            // Aktualizacja liczby klientów w kolejce
            sem_op.sem_num = SEM_ACCESS;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            
            if (semop(data->sem_id, &sem_op, 1) == -1) {
                handle_error(ERROR_SEMAPHORE, "Klient nie może zablokować semafora dostępu");
                shmdt(shm);
                free(data);
                pthread_exit(NULL);
            }
            
            if (shm->customers_in_queue[register_id] > 0) {
                shm->customers_in_queue[register_id]--;
            }
            
            sem_op.sem_num = SEM_ACCESS;
            sem_op.sem_op = 1;
            sem_op.sem_flg = 0;
            
            if (semop(data->sem_id, &sem_op, 1) == -1) {
                handle_error(ERROR_SEMAPHORE, "Klient nie może zwolnić semafora dostępu");
                shmdt(shm);
                free(data);
                pthread_exit(NULL);
            }
        }
    }
    
    // Wysłanie wiadomości o opuszczeniu supermarketu
    msg.mtype = MSG_CUSTOMER_LEAVES;
    msg.customer_id = data->id;
    
    if (msgsnd(data->msg_id, &msg, sizeof(Message) - sizeof(long), 0) == -1) {
        handle_error(ERROR_MESSAGE_QUEUE, "Nie można wysłać wiadomości o opuszczeniu supermarketu");
    }
    
    printf("KLIENT %d: Opuszczam supermarket.\n", data->id);
    
    // Odłączenie pamięci dzielonej
    if (shmdt(shm) == -1) {
        handle_error(ERROR_MEMORY, "Klient nie może odłączyć pamięci dzielonej");
    }
    
    free(data);
    pthread_exit(NULL);
}

// Główna funkcja procesu klienta
void customer_process(int num_customers) {
    int shm_id, sem_id, msg_id;
    pthread_t* customer_threads;
    
    // Inicjalizacja generatora liczb losowych
    srand(time(NULL));
    
    // Pobierz identyfikator pamięci dzielonej
    if ((shm_id = shmget(SHM_KEY, sizeof(SharedMemory), 0666)) == -1) {
        handle_error(ERROR_MEMORY, "Klient nie może uzyskać dostępu do pamięci dzielonej");
        return;
    }
    
    // Pobierz identyfikator semaforów
    if ((sem_id = semget(SEM_KEY, SEM_TOTAL, 0666)) == -1) {
        handle_error(ERROR_SEMAPHORE, "Klient nie może uzyskać dostępu do semaforów");
        return;
    }
    
    // Pobierz identyfikator kolejki komunikatów
    if ((msg_id = msgget(MSG_KEY, 0666)) == -1) {
        handle_error(ERROR_MESSAGE_QUEUE, "Klient nie może uzyskać dostępu do kolejki komunikatów");
        return;
    }
    
    // Alokacja pamięci dla wątków klientów
    customer_threads = (pthread_t*)malloc(num_customers * sizeof(pthread_t));
    if (customer_threads == NULL) {
        handle_error(ERROR_MEMORY, "Nie można zaalokować pamięci dla wątków klientów");
        return;
    }
    
    printf("PROCES KLIENTA: Tworzenie %d klientów.\n", num_customers);
    
    // Tworzenie wątków klientów
    for (int i = 0; i < num_customers; i++) {
        CustomerData* data = (CustomerData*)malloc(sizeof(CustomerData));
        if (data == NULL) {
            handle_error(ERROR_MEMORY, "Nie można zaalokować pamięci dla danych klienta");
            continue;
        }
        
        data->id = i + 1;
        data->shopping_time = (rand() % 10) + 5; // 5-15 sekund na zakupy
        data->shm_id = shm_id;
        data->sem_id = sem_id;
        data->msg_id = msg_id;
        
        if (pthread_create(&customer_threads[i], NULL, customer_thread, (void*)data) != 0) {
            handle_error(ERROR_THREAD, "Nie można utworzyć wątku klienta");
            free(data);
            continue;
        }
        
        // Małe opóźnienie między tworzeniem klientów
        //usleep(200000); // 0.2 sekundy
        sleep(1); // 1 sekunda


    }
    
    // Oczekiwanie na zakończenie wszystkich wątków klientów
    for (int i = 0; i < num_customers; i++) {
        pthread_join(customer_threads[i], NULL);
    }
    
    free(customer_threads);
    printf("PROCES KLIENTA: Wszyscy klienci opuścili supermarket.\n");
}
