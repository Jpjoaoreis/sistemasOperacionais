#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_mutex_t lock;

int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
int is_safe_state();

void* customer_thread(void* arg) {
    int customer_num = *(int*)arg;
    int request[NUMBER_OF_RESOURCES];
    int release[NUMBER_OF_RESOURCES];

    while (1) {
        // Solicita recursos aleatórios limitados por "need"
        pthread_mutex_lock(&lock);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
            request[i] = rand() % (need[customer_num][i] + 1);
        pthread_mutex_unlock(&lock);

        if (request_resources(customer_num, request) == 0) {
            sleep(1); // Usa os recursos

            // Libera parte ou todos os recursos
            pthread_mutex_lock(&lock);
            for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
                release[i] = rand() % (allocation[customer_num][i] + 1);
            pthread_mutex_unlock(&lock);

            release_resources(customer_num, release);
        }

        sleep(1);
    }

    return NULL;
}

int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&lock);

    // Checa se a requisição é válida
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    // Aloca temporariamente
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    // Verifica se o sistema permanece seguro
    if (!is_safe_state()) {
        // Desfaz alocação
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&lock);
        return -1;
    }

    pthread_mutex_unlock(&lock);
    return 0;
}

int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&lock);

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (release[i] > allocation[customer_num][i]) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }

    pthread_mutex_unlock(&lock);
    return 0;
}

int is_safe_state() {
    int work[NUMBER_OF_RESOURCES];
    int finish[NUMBER_OF_CUSTOMERS] = {0};

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        work[i] = available[i];

    for (int count = 0; count < NUMBER_OF_CUSTOMERS; count++) {
        int found = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++)
                    if (need[i][j] > work[j]) break;

                if (j == NUMBER_OF_RESOURCES) {
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++)
                        work[k] += allocation[i][k];
                    finish[i] = 1;
                    found = 1;
                }
            }
        }
        if (!found) return 0;
    }

    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != NUMBER_OF_RESOURCES + 1) {
        fprintf(stderr, "Uso: %s <recurso1> <recurso2> ...\n", argv[0]);
        return -1;
    }

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        available[i] = atoi(argv[i + 1]);

    srand(time(NULL));

    // Inicializa maximum, allocation, need
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }

    pthread_mutex_init(&lock, NULL);

    pthread_t threads[NUMBER_OF_CUSTOMERS];
    int ids[NUMBER_OF_CUSTOMERS];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, customer_thread, &ids[i]);
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&lock);

    return 0;
}
