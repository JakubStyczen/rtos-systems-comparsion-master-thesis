#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

#define ITER 1000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

uint64_t start[ITER];
uint64_t finish[ITER];
volatile int count = 0;

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void *thread1_func(void *arg)
{
    while (count < ITER)
    {
        pthread_mutex_lock(&mutex);
        start[count] = now_ns();
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *thread2_func(void *arg)
{
    while (count < ITER)
    {
        pthread_mutex_lock(&mutex);
        finish[count] = now_ns();
        count++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    FILE *f;
    f = fopen("linux_mutexes.csv", "w");
    if (!f)
        return -1;

    for (int i = 0; i < ITER; i++)
    {
        fprintf(f, "%ld;%ld\n",
                start[i],
                finish[i]);
    }

    fclose(f);
    return 0;
}
