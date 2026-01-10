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
        printf("First");
        pthread_mutex_unlock(&mutex);
        // usleep(1);
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
        printf("Second");
        pthread_mutex_unlock(&mutex);
        usleep(1);
    }
    return NULL;
}

static void pin_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}



int main(void)
{
    pthread_t t1, t2;

    /* przypnij proces do jednego rdzenia */
    pin_to_cpu(0);

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    FILE *f;
    f = fopen("mutex_linux.csv", "w");
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
