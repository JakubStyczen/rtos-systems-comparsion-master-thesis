#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define SAMPLES 100000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv2 = PTHREAD_COND_INITIALIZER;

int turn = 1;
int count = 0;

uint64_t start[SAMPLES];
uint64_t finish[SAMPLES];

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void *thread1_func(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);

        while (turn != 1 && count < SAMPLES)
            pthread_cond_wait(&cv1, &mutex);

        if (count >= SAMPLES)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        start[count] = now_ns();

        turn = 2;
        pthread_cond_signal(&cv2);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *thread2_func(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);

        while (turn != 2 && count < SAMPLES)
            pthread_cond_wait(&cv2, &mutex);

        if (count >= SAMPLES)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        finish[count] = now_ns();
        count++;
        turn = 1;
        pthread_cond_signal(&cv1);
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

    FILE *f = fopen("linux_condvar.csv", "w");
    if (!f)
        return -1;

    for (int i = 0; i < SAMPLES; i++)
        fprintf(f, "%lu;%lu\n", start[i], finish[i]);

    fclose(f);
    return 0;
}
