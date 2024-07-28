#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 3

pthread_mutex_t lock;
pthread_cond_t cond;
int turn = 1;

void *func1(void *arg) {
    char *msg = (char *)arg;

    pthread_mutex_lock(&lock);
    while (turn != 1) {
        pthread_cond_wait(&cond, &lock);
    }
    printf("Thread 1: %s\n", msg);
    printf("Thread 1 ID: %ld\n", pthread_self());
    turn = 2;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

void *func2(void *arg) {
    char *msg = (char *)arg;

    pthread_mutex_lock(&lock);
    while (turn != 3) {
        pthread_cond_wait(&cond, &lock);
    }
    printf("Thread 2: %s\n", msg);
    printf("Thread 2 ID: %ld\n", pthread_self());
    turn = 4;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

void *func3(void *arg) {
    char *msg = (char *)arg;

    pthread_mutex_lock(&lock);
    while (turn != 2) {
        pthread_cond_wait(&cond, &lock);
    }
    printf("Thread 3: %s\n", msg);
    printf("Thread 3 ID: %ld\n", pthread_self());
    turn = 3;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    char *msg = "Welcome to Threads-COMP 8567";
    int i;

    // Array of function pointers
    void* (*funcs[NUM_THREADS])(void *) = {func1, func2, func3};

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // Create all threads using a for loop
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, funcs[i], (void *)msg);
    }

    // Use a for loop to join all threads
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Main thread ID: %ld\n", pthread_self());

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}