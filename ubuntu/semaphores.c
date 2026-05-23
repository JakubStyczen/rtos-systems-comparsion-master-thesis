#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define SAMPLES 1000
#define NUM_TIMERS 5

/* okresy: 1,2,5,10,100 ms */
const uint64_t periods_ns[NUM_TIMERS] = {
    1000000ULL,
    2000000ULL,
    5000000ULL,
    10000000ULL,
    100000000ULL};

sem_t sems[NUM_TIMERS];

typedef struct
{
    uint64_t period_ns;
    struct timespec ts[SAMPLES];
    int idx;
} timer_data_t;

timer_data_t timers[NUM_TIMERS];

static timer_t master_timer;

static void timer_handler(int sig, siginfo_t *si, void *uc)
{
    static uint64_t tick = 0;
    tick++;

    uint64_t elapsed_ns = tick * 1000000ULL;

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        if (elapsed_ns % timers[i].period_ns == 0)
        {
            sem_post(&sems[i]);
        }
    }
}

void *timer_thread(void *arg)
{
    timer_data_t *t = (timer_data_t *)arg;

    while (t->idx < SAMPLES)
    {
        sem_wait(&sems[(t - timers)]);

        clock_gettime(CLOCK_REALTIME, &t->ts[t->idx]);
        t->idx++;
    }

    return NULL;
}

int main()
{
    struct sigaction sa;
    struct sigevent sev;
    struct itimerspec its;
    pthread_t threads[NUM_TIMERS];

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        sem_init(&sems[i], 0, 0);
        timers[i].period_ns = periods_ns[i];
        timers[i].idx = 0;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigaction(SIGRTMIN, &sa, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &master_timer;

    timer_create(CLOCK_REALTIME, &sev, &master_timer);

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 1000000;

    timer_settime(master_timer, 0, &its, NULL);

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        pthread_create(&threads[i], NULL, timer_thread, &timers[i]);
    }

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    FILE *f = fopen("linux_semaphores.csv", "w");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    for (int i = 0; i < SAMPLES; i++)
    {
        for (int t = 0; t < NUM_TIMERS; t++)
        {
            uint64_t period_ms = timers[t].period_ns / 1000000ULL;
            fprintf(f, "%llums;%llu\n",
                    (unsigned long long)period_ms,
                    (unsigned long long)time_ns);
        }
    }

    fclose(f);

    return 0;
}
