#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 10

int num = 120;
pthread_mutex_t lock;

void* decrement(void* arg) {
    int mode = *((int*)arg);
    pthread_t thread_id = pthread_self();

    pthread_mutex_lock(&lock);
    if (mode == PTHREAD_CREATE_DETACHED) {
        num -= 4;
        printf("Thread ID: %ld (Detached) - num: %d\n", thread_id, num);
    } else {
        num -= 3;
        printf("Thread ID: %ld (Joinable) - num: %d\n", thread_id, num);
    }
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;
    int rc;
    int i;
    int detach_state;

    pthread_mutex_init(&lock, NULL);

    for (i = 0; i < NUM_THREADS; i++) {
        if (i % 2 == 0) {
            detach_state = PTHREAD_CREATE_JOINABLE;
        } else {
            detach_state = PTHREAD_CREATE_DETACHED;
        }

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, detach_state);

        if (detach_state == PTHREAD_CREATE_JOINABLE) {
            rc = pthread_create(&threads[i], &attr, decrement, &detach_state);
            if (rc) {
                printf("ERROR: pthread_create() return code is %d\n", rc);
                exit(-1);
            }
        } else {
            rc = pthread_create(&threads[i], &attr, decrement, &detach_state);
            if (rc) {
                printf("ERROR: pthread_create() return code is %d\n", rc);
                exit(-1);
            }
        }

        pthread_attr_destroy(&attr);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (i % 2 == 0) {
            rc = pthread_join(threads[i], NULL);
            if (rc) {
                printf("ERROR: pthread_join() return code is %d\n", rc);
                exit(-1);
            }
        }
    }

    pthread_mutex_destroy(&lock);

    printf("Final value of num: %d\n", num);
    pthread_exit(NULL);
}