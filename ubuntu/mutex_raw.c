#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>

#define ITER 1000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

uint64_t start[ITER];
uint64_t finish[ITER];

int turn = 1;
int count = 0;

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void pin_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void set_fifo()
{
    struct sched_param param;
    param.sched_priority = 80;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0)
    {
        perror("sched");
        exit(1);
    }
}

void *thread1_func(void *arg)
{
    // pin_to_cpu(0);
    // set_fifo();

    while (1)
    {
        pthread_mutex_lock(&mutex);

        if (count >= ITER) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (turn == 1)
        {
            start[count] = now_ns();
            // printf("Wątek 1: mam mutex %ld\n", count);
            turn = 2;
        }

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void *thread2_func(void *arg)
{
    // pin_to_cpu(0);
    // set_fifo();

    while (1)
    {
        pthread_mutex_lock(&mutex);

        if (count >= ITER) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (turn == 2)
        {
            finish[count] = now_ns();
            // printf("Wątek 2: mam mutex %ld\n", count);
            count++;
            turn = 1;
        }

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

    FILE *f = fopen("linux_mutex_raw.csv", "w");
    for (int i = 0; i < ITER; i++)
        fprintf(f, "%lu;%lu\n", start[i], finish[i]);

    fclose(f);
    return 0;
}
