#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>

volatile atomic_int done = 0;  // globalny sygnał zakończenia

#define SAMPLES 1000
#define NUM_TIMERS 5

const uint64_t periods_ns[NUM_TIMERS] = {1000000ULL, 2000000ULL, 5000000ULL, 10000000ULL, 100000000ULL}; // 1,2,5,10,100ms

typedef struct {
    uint64_t period_ns;
    struct timespec ts[SAMPLES];
} timer_data_t;

static inline void timespec_add_ns(struct timespec *t, uint64_t ns)
{
    t->tv_nsec += ns;
    while (t->tv_nsec >= 1000000000L) {
        t->tv_nsec -= 1000000000L;
        t->tv_sec++;
    }
}

void* timer_thread(void *arg)
{
    timer_data_t *data = (timer_data_t*)arg;
    struct timespec next;
    int idx = 0;

    clock_gettime(CLOCK_MONOTONIC, &next);

    while (!done) {  // wątek działa dopóki globalny sygnał nie jest ustawiony
        timespec_add_ns(&next, data->period_ns);

        int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        if (ret != 0) {
            perror("clock_nanosleep");
            break;
        }

        if (idx < SAMPLES) {  // zapis tylko jeśli tablica niepełna
            clock_gettime(CLOCK_MONOTONIC, &data->ts[idx]);
            idx++;

            // jeśli to timer 100ms i tablica pełna, ustawiamy done
            if (data->period_ns == 100000000ULL && idx == SAMPLES) {
                done = 1;
            }
        }
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_TIMERS];
    timer_data_t timers[NUM_TIMERS];

    // initialize timer data
    for (int i = 0; i < NUM_TIMERS; i++) {
        timers[i].period_ns = periods_ns[i];
    }

    // start threads
    for (int i = 0; i < NUM_TIMERS; i++) {
        pthread_create(&threads[i], NULL, timer_thread, &timers[i]);
    }

    // wait for completion
    for (int i = 0; i < NUM_TIMERS; i++) {
        pthread_join(threads[i], NULL);
    }

    // open file for writing
    FILE *f = fopen("output_multitimer_parrael.csv", "w");
    if (!f) {
        perror("fopen");
        return 1;
    }

    // dump all results in Xms;timestamp format
    for (int i = 0; i < NUM_TIMERS; i++) {
        for (int j = 0; j < SAMPLES; j++) {
            uint64_t ns = (uint64_t)timers[i].ts[j].tv_sec * 1000000000ULL + timers[i].ts[j].tv_nsec;
            fprintf(f, "%llums;%llu\n",
                    (unsigned long long)(timers[i].period_ns / 1000000ULL),
                    (unsigned long long)ns);
        }
    }

    fclose(f);
    return 0;
}
