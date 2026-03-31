#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define SAMPLES 100000
#define NUM_TIMERS 1

volatile float x = 1.001f;

void cpu_load_fpu(void)
{
    for (int i = 0; i < 50; i++)
    {
        for (int j = 0; j < 10000; j++)
        {
            x = x * 1.000001f + 0.000001f;
        }
    }
}

/* okresy: 1,2,5,10,100 ms */
const uint64_t periods_ns[NUM_TIMERS] = {
    1000000ULL,
};

sem_t sems[NUM_TIMERS];

typedef struct {
    uint64_t period_ns;
    struct timespec ts[SAMPLES];
    int idx;
    int id;
} timer_data_t;

timer_data_t timers[NUM_TIMERS];

static timer_t master_timer;
volatile int finished = 0;
/* ================= MASTER CLOCK ISR ================= */

static void timer_handler(int sig, siginfo_t *si, void *uc)
{
    static uint64_t tick = 0;
    tick++;  // 1 tick = 1 ms

    uint64_t elapsed_ns = tick * 1000000ULL;

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        if (elapsed_ns % timers[i].period_ns == 0)
        {
            sem_post(&sems[i]);
        }
    }
}

/* ================= THREAD ================= */

// void *timer_thread(void *arg)
// {
//     timer_data_t *t = (timer_data_t *)arg;

//     while (1)
//     {
//         sem_wait(&sems[(t - timers)]);
//         printf("%d, %d\n", t->id, t->idx);
//         if (t->idx < SAMPLES){
//         clock_gettime(CLOCK_REALTIME, &t->ts[t->idx]);
//         t->idx++;
//         // printf("%d, %d\n", t->id, t->idx);
//     } else if ((t->id == 4) && (t->idx >= SAMPLES)) {
//             printf("koniec");
            
//             break;
//         }
//     }

//     return NULL;
// }


void *timer_thread(void *arg)
{
    timer_data_t *t = (timer_data_t *)arg;

    while (!finished)
    {
        sem_wait(&sems[t->id]);
        // printf("%d, %d\n", t->id, t->idx);
        cpu_load_fpu();
        if (t->idx < SAMPLES)
        {
            clock_gettime(CLOCK_REALTIME, &t->ts[t->idx]);
            // printf("%d, %d\n", t->id, t->idx);
            t->idx++;
        }

        if (t->id == 0 && t->idx >= SAMPLES)
        {
            finished = 1;

            /* obudź wszystkie wątki żeby mogły wyjść */
            for (int i = 0; i < NUM_TIMERS; i++)
                sem_post(&sems[i]);
        }
    }

    return NULL;
}

/* ================= MAIN ================= */

int main()
{
    struct sigaction sa;
    struct sigevent sev;
    struct itimerspec its;
    pthread_t threads[NUM_TIMERS];

    /* init semafory */
    for (int i = 0; i < NUM_TIMERS; i++)
    {
        sem_init(&sems[i], 0, 0);
        timers[i].period_ns = periods_ns[i];
        timers[i].idx = 0;
        timers[i].id = i;
    }

    /* konfiguracja handlera */
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigaction(SIGRTMIN, &sa, NULL);

    /* konfiguracja timera */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &master_timer;

    timer_create(CLOCK_REALTIME, &sev, &master_timer);

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1000000;      // 1 ms
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 1000000;   // 1 ms

    timer_settime(master_timer, 0, &its, NULL);

    /* start wątków */
    for (int i = 0; i < NUM_TIMERS; i++)
    {
        pthread_create(&threads[i], NULL, timer_thread, &timers[i]);
    }

    /* czekaj */
    for (int i = 0; i < NUM_TIMERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /* dump wyników */
FILE *f = fopen("linux_semaphores_1ms_load.csv", "w");
if (!f) {
    perror("fopen");
    return 1;
}

for (int i = 0; i < SAMPLES; i++)
{
    for (int t = 0; t < NUM_TIMERS; t++)
    {
        /* konwersja okresu z ns na ms */
        uint64_t period_ms = timers[t].period_ns / 1000000ULL;

        /* zapis czasu jako absolutne ns */
        uint64_t time_ns =
            (uint64_t)timers[t].ts[i].tv_sec * 1000000000ULL +
            timers[t].ts[i].tv_nsec;

        fprintf(f, "%llums;%llu\n",
                (unsigned long long)period_ms,
                (unsigned long long)time_ns);
    }
}

fclose(f);
// printf("Zapisano results.csv\n");



    return 0;
}
