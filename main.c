#include "common.h"
#include "utils.h"

// Deklaracje funkcji importowanych z innych modułów
extern void manager_process(int customers_per_register);
extern void customer_process(int num_customers);
extern void fireman_process();

int main(int argc, char *argv[]) {
    pid_t manager_pid, customer_pid, fireman_pid;
    int status;
    int customers_per_register = 5; // domyślna wartość K
    int num_customers = 20;         // domyślna liczba klientów
    
    // Sprawdzenie argumentów wiersza poleceń
    if (argc > 1) {
        customers_per_register = atoi(argv[1]);
    }
    
    if (argc > 2) {
        num_customers = atoi(argv[2]);
    }
    
    // Walidacja danych wejściowych
    if (!validate_input(customers_per_register, num_customers)) {
        fprintf(stderr, "Nieprawidłowe dane wejściowe. Używam wartości domyślnych.\n");
        customers_per_register = 5;
        num_customers = 20;
    }
    
    printf("Parametry symulacji:\n");
    printf("- Liczba klientów na kasę (K): %d\n", customers_per_register);
    printf("- Liczba klientów: %d\n", num_customers);
    printf("- Minimalnie czynne kasy: %d\n", MIN_REGISTERS);
    printf("- Maksymalna liczba kas: %d\n", MAX_REGISTERS);
    
    // Inicjalizacja obsługi sygnałów
    setup_signal_handlers();
    
    // Czyszczenie zasobów w przypadku ich poprzedniego istnienia
    cleanup_resources();
    
    // Tworzenie procesu kierownika supermarketu
    manager_pid = fork();
    if (manager_pid == -1) {
        handle_error(ERROR_FORK, "Nie można utworzyć procesu kierownika");
        return EXIT_FAILURE;
    } else if (manager_pid == 0) {
        // Proces potomny - kierownik
        manager_process(customers_per_register);
        exit(EXIT_SUCCESS);
    }
    
    // Małe opóźnienie, aby kierownik zdążył zainicjalizować supermarket
    sleep(1);
    
    // Tworzenie procesu strażaka
    fireman_pid = fork();
    if (fireman_pid == -1) {
        handle_error(ERROR_FORK, "Nie można utworzyć procesu strażaka");
        kill(manager_pid, SIGTERM);
        return EXIT_FAILURE;
    } else if (fireman_pid == 0) {
        // Proces potomny - strażak
        fireman_process();
        exit(EXIT_SUCCESS);
    }
    
    // Tworzenie procesu klienta (główny proces tworzący wątki klientów)
    customer_pid = fork();
    if (customer_pid == -1) {
        handle_error(ERROR_FORK, "Nie można utworzyć procesu klienta");
        kill(manager_pid, SIGTERM);
        kill(fireman_pid, SIGTERM);
        return EXIT_FAILURE;
    } else if (customer_pid == 0) {
        // Proces potomny - klient
        customer_process(num_customers);
        exit(EXIT_SUCCESS);
    }
    
    // Proces główny czeka na zakończenie wszystkich procesów potomnych
    printf("Proces główny czeka na zakończenie procesów potomnych.\n");
    waitpid(customer_pid, &status, 0);
    printf("Proces klienta zakończył działanie.\n");
    
    // Po zakończeniu wszystkich klientów, wyślij sygnał zakończenia do menedżera i strażaka
    kill(manager_pid, SIGTERM);
    kill(fireman_pid, SIGTERM);
    
    waitpid(manager_pid, &status, 0);
    printf("Proces kierownika zakończył działanie.\n");
    
    waitpid(fireman_pid, &status, 0);
    printf("Proces strażaka zakończył działanie.\n");
    
    // Czyszczenie zasobów systemowych
    cleanup_resources();
    
    printf("Symulacja supermarketu zakończona pomyślnie.\n");
    return EXIT_SUCCESS;
}
