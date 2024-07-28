#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *func1(void *arg) {
    char *msg = (char *)arg;
    printf("Thread 1: %s\n", msg);
    printf("Thread 1 ID: %lu\n", pthread_self());
    return NULL;
}

void *func2(void *arg) {
    char *msg = (char *)arg;
    printf("Thread 2: %s\n", msg);
    printf("Thread 2 ID: %lu\n", pthread_self());
    return NULL;
}

void *func3(void *arg) {
    char *msg = (char *)arg;
    printf("Thread 3: %s\n", msg);
    printf("Thread 3 ID: %lu\n", pthread_self());
    return NULL;
}

int main() {
    pthread_t threads[3];
    char *msg = "Welcome to Threads-COMP 8567";

    // Create threads
    pthread_create(&threads[0], NULL, func1, (void *)msg);
    pthread_create(&threads[1], NULL, func2, (void *)msg);
    pthread_create(&threads[2], NULL, func3, (void *)msg);

    // Join threads in desired order
    pthread_join(threads[0], NULL);  // Wait for thread 1 to finish
    pthread_join(threads[2], NULL);  // Wait for thread 3 to finish
    pthread_join(threads[1], NULL);  // Wait for thread 2 to finish

    printf("Main thread ID: %lu\n", pthread_self());
    return 0;
}