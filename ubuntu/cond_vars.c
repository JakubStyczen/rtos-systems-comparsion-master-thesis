#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

#define ITER 1000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

uint64_t start[ITER];
uint64_t finish[ITER];

volatile int count = 0;
volatile int ready = 0;

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* Wątek A – sygnalizujący */
void *thread_signal(void *arg)
{
    while (count < ITER)
    {
        pthread_mutex_lock(&mutex);

        ready = 1;
        start[count] = now_ns();
        printf("a");
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

/* Wątek B – czekający */
void *thread_wait(void *arg)
{
    while (count < ITER)
    {
        pthread_mutex_lock(&mutex);

        while (!ready)
            pthread_cond_wait(&cond, &mutex);

        finish[count] = now_ns();
        ready = 0;
        count++;
        printf("B");
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    // pin_to_cpu(0);

    pthread_create(&t1, NULL, thread_signal, NULL);
    pthread_create(&t2, NULL, thread_wait, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    FILE *f;
    f = fopen("linux_cond_vars_printf.csv", "w");
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
