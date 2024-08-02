#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t global_mutex;
int shared_var = 120;

void* modify_var(void* arg) {
    int detach_state = *((int*)arg);
    pthread_t tid = pthread_self();

    pthread_mutex_lock(&global_mutex);
    if (detach_state == PTHREAD_CREATE_DETACHED) {
        shared_var -= 4;
        printf("Thread ID: %lu (Detached) - shared_var: %d\n", tid, shared_var);
    } else {
        shared_var -= 3;
        printf("Thread ID: %lu (Joinable) - shared_var: %d\n", tid, shared_var);
    }
    pthread_mutex_unlock(&global_mutex);

    free(arg); // Free the allocated memory
    return NULL;
}

int main() {
    pthread_t threads[10];
    pthread_attr_t thread_attr;
    int i;

    pthread_mutex_init(&global_mutex, NULL);

    for (i = 0; i < 10; i++) {
        pthread_attr_init(&thread_attr);
        int* detach_state = malloc(sizeof(int));
        if (detach_state == NULL) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }

        if (i % 2 == 0) { // Create joinable thread for even indices
            *detach_state = PTHREAD_CREATE_JOINABLE;
            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
        } else { // Create detached thread for odd indices
            *detach_state = PTHREAD_CREATE_DETACHED;
            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        }

        if (pthread_create(&threads[i], &thread_attr, modify_var, detach_state) != 0) {
            perror("Error creating thread");
            free(detach_state); // Free memory if thread creation fails
        }

        pthread_attr_destroy(&thread_attr);
    }

    for (i = 0; i < 10; i += 2) { // Wait only for joinable threads
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
        }
    }

    pthread_mutex_destroy(&global_mutex);

    printf("Final value of shared_var: %d\n", shared_var);
    return 0;
}