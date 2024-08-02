#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 10
 int isProgrammRunning = 1;
int num = 120;
pthread_mutex_t lock;

void decrement_num(int decrement_value, const char* thread_type, pthread_t thread_id) {
    pthread_mutex_lock(&lock);
    num -= decrement_value;
    printf("Thread ID: %ld (%s) - num: %d\n", thread_id, thread_type, num);
    pthread_mutex_unlock(&lock);
}

void* decrement_detached(void* arg) {
    pthread_t thread_id = pthread_self();
    decrement_num(4, "Detached", thread_id);
    pthread_exit(NULL);
}

void* decrement_joinable(void* arg) {
    pthread_t thread_id = pthread_self();
    decrement_num(3, "Joinable", thread_id);
    pthread_exit(NULL);
}

void initialize_thread_attributes(pthread_attr_t* attr, int detach_state) {
    pthread_attr_init(attr);
    pthread_attr_setdetachstate(attr, detach_state);
}

void create_thread(pthread_t* thread, pthread_attr_t* attr, void* (*decrement_func)(void*), int detach_state) {
    initialize_thread_attributes(attr, detach_state);
    pthread_create(thread, attr, decrement_func, NULL);
    pthread_attr_destroy(attr);
}

void create_threads(int i, pthread_t threads[], pthread_attr_t attrs[], void* (*decrement[])(void*)) {
    if (i >= NUM_THREADS && isProgrammRunning==1) {
        return;
    }

    int detach_state = (i % 2 == 0) ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED;
    create_thread(&threads[i], &attrs[i], decrement[i % 2], detach_state);
    create_threads(i + 1, threads, attrs, decrement);
}

void join_thread(pthread_t thread) {
    pthread_join(thread, NULL);
}

void join_threads(int i, pthread_t threads[]) {
    if (i >= NUM_THREADS) {
        return;
    }

    (i % 2 == 0) ? join_thread(threads[i]) : (void)0;
    join_threads(i + 1, threads);
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attrs[NUM_THREADS];
    void* (*decrement[])(void*) = {decrement_joinable, decrement_detached};

    pthread_mutex_init(&lock, NULL);

    create_threads(0, threads, attrs, decrement);
    join_threads(0, threads);

    pthread_mutex_destroy(&lock);

    printf("Final value of num: %d\n", num);
    pthread_exit(NULL);
}