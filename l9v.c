#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int isProgramRunning = 1;

void *func1(void *arg) {
    char *msg = (char *)arg;
    int i = 0;
    while (i < 1) {
        if (i == 0) {
            printf("Thread 1: %s\n", msg);
            printf("Thread 1 ID: %ld\n", pthread_self());
        } else {
            printf("Thread 1: Loop iteration %d\n", i);
        }
        i++;
    }
    pthread_exit(NULL);
}

void *func2(void *arg) {
    char *msg = (char *)arg;
    int i = 0;
    while (i < 1) {
        if (i == 0) {
            printf("Thread 2: %s\n", msg);
            printf("Thread 2 ID: %ld\n", pthread_self());
        } else {
            printf("Thread 2: Loop iteration %d\n", i);
        }
        i++;
    }
    pthread_exit(NULL);
}

void *func3(void *arg) {
    char *msg = (char *)arg;
    int i = 0;
    while (i < 1) {
        if (i == 0) {
            printf("Thread 3: %s\n", msg);
            printf("Thread 3 ID: %ld\n", pthread_self());
        } else {
            printf("Thread 3: Loop iteration %d\n", i);
        }
        i++;
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2, t3;
    char *msg = "Welcome to Threads-COMP 8567";
    int thread_created = 0;

    while (thread_created < 3 && isProgramRunning==1) {
        if (thread_created == 0) {
            pthread_create(&t1, NULL, func1, (void *)msg);
            pthread_join(t1, NULL);
            thread_created++;
        } else if (thread_created == 1) {
            pthread_create(&t3, NULL, func3, (void *)msg);
            pthread_join(t3, NULL);
            thread_created++;
        } else if (thread_created == 2) {
            pthread_create(&t2, NULL, func2, (void *)msg);
            pthread_join(t2, NULL);
            thread_created++;
        }
    }

    printf("Main thread ID: %ld\n", pthread_self());

    return 0;
}