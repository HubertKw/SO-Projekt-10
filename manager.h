#ifndef MANAGER_H
#define MANAGER_H

// Deklaracja funkcji inicjalizującej managera
int initialize_manager(int shm_id, int sem_id, int initial_cashiers);

// Deklaracje funkcji blokowania/odblokowania semafora
void sem_lock(int sem_id);
void sem_unlock(int sem_id);

#endif
