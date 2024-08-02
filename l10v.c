#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 10

pthread_mutex_t lock;
int num = 120;

void *decrement(void *arg) {
    int mode = *((int *)arg);
    free(arg);
    
    if (mode == PTHREAD_CREATE_DETACHED) {
        pthread_mutex_lock(&lock);
        num -= 4;
        pthread_mutex_unlock(&lock);
        printf("Detached Thread ID: %ld, num: %d\n", pthread_self(), num);
    } else {
pthread_mutex_lock(&lock);
        num -= 3;
        pthread_mutex_unlock(&lock);
        printf("Joinable Thread ID: %ld, num: %d\n", pthread_self(), num);
    }
    
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;
    int rc;
    int i = 0;
    
    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    do {
        int *mode = malloc(sizeof(int));
pthread_attr_init(&attr);
        switch (i % 2) {
            case 0:
                *mode = PTHREAD_CREATE_JOINABLE;
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                rc = pthread_create(&threads[i], &attr, decrement, mode);
                if (rc) {
                    printf("Error: unable to create thread, %d\n", rc);
                    exit(-1);
                }
                break;
            case 1:
                *mode = PTHREAD_CREATE_DETACHED;
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                rc = pthread_create(&threads[i], &attr, decrement, mode);
                if (rc) {
                    printf("Error: unable to create thread, %d\n", rc);
                    exit(-1);
                }
 break;
        }
        pthread_attr_destroy(&attr);
        i++;
    } while (i < NUM_THREADS);

    // Wait for joinable threads
    i = 0;
    do {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            printf("Error: unable to join thread, %d\n", rc);
            exit(-1);
        }
        i += 2;
    } while (i < NUM_THREADS);

    // Destroy mutex
    pthread_mutex_destroy(&lock);

    printf("Final value of num: %d\n", num);
    pthread_exit(NULL);
}