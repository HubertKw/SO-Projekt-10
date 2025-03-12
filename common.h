#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

// Definicje stałych
#define MAX_CUSTOMERS 100
#define MAX_REGISTERS 10
#define MIN_REGISTERS 2
#define SHM_KEY 1234
#define SEM_KEY 5678
#define MSG_KEY 9012
#define FIFO_PATH "/tmp/supermarket_fifo"
#define FIRE_SIGNAL SIGUSR1
#define CUSTOMER_DONE_SIGNAL SIGUSR2

// Definicje błędów
#define ERROR_INIT 1
#define ERROR_FORK 2
#define ERROR_MEMORY 3
#define ERROR_SEMAPHORE 4
#define ERROR_MESSAGE_QUEUE 5
#define ERROR_SIGNAL 6
#define ERROR_FIFO 7
#define ERROR_THREAD 8

// Indeksy semaforów
#define SEM_ACCESS 0     // Do synchronizacji dostępu do pamięci dzielonej
#define SEM_MANAGER 1    // Do synchronizacji menedżera
#define SEM_CUSTOMER 2   // Do synchronizacji klientów
#define SEM_TOTAL 3      // Całkowita liczba semaforów

// Typy wiadomości dla kolejki komunikatów
#define MSG_NEW_CUSTOMER 1
#define MSG_CUSTOMER_LEAVES 2
#define MSG_REGISTER_OPEN 3
#define MSG_REGISTER_CLOSE 4
#define MSG_FIRE_ALARM 5

// Struktura pamięci dzielonej
typedef struct {
    int active_customers;     // Liczba klientów w supermarkecie
    int active_registers;     // Liczba czynnych kas
    int customers_per_register; // Wartość K (na ilu klientów przypada jedna kasa)
    int fire_alarm;           // Flaga alarmu pożarowego
    int customers_in_queue[MAX_REGISTERS]; // Liczba klientów w kolejce do każdej kasy
    int register_status[MAX_REGISTERS];    // Status kas (0 - zamknięta, 1 - otwarta)
} SharedMemory;

// Struktura wiadomości dla kolejki komunikatów
typedef struct {
    long mtype;              // Typ wiadomości
    int customer_id;         // ID klienta
    int register_id;         // ID kasy
    int action;              // Dodatkowa informacja o akcji
} Message;

// Funkcja do obsługi błędów
void handle_error(int error_code, const char* message);

#endif // COMMON_H
