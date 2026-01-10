#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#define SAMPLES 1000
#define NUM_TIMERS 5

const uint64_t periods_ns[NUM_TIMERS] = {1000000ULL, 2000000ULL, 5000000ULL, 10000000ULL, 100000000ULL}; // 1,2,5,10,100ms

typedef struct {
    uint64_t period_ns;
    struct timespec ts[SAMPLES];
    int idx;
} timer_data_t;

timer_data_t timers[NUM_TIMERS];
timer_t timer_ids[NUM_TIMERS];

atomic_int done = 0;

void timer_handler(union sigval sv) {
    int id = (int)(intptr_t)sv.sival_ptr;
    timer_data_t *t = &timers[id];

    if (t->idx < SAMPLES) {
        clock_gettime(CLOCK_REALTIME, &t->ts[t->idx]);  // <- tutaj używamy REALTIME
        t->idx++;

        if (t->period_ns == 100000000ULL && t->idx == SAMPLES) {
            done = 1;
        }
    }
}

int main(void) {
    struct sigevent sev;
    struct itimerspec its;

    for (int i = 0; i < NUM_TIMERS; i++) {
        timers[i].period_ns = periods_ns[i];
        timers[i].idx = 0;

        memset(&sev, 0, sizeof(sev));
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = timer_handler;
        sev.sigev_value.sival_ptr = (void*)(intptr_t)i;

        if (timer_create(CLOCK_REALTIME, &sev, &timer_ids[i]) == -1) { // REALTIME
            perror("timer_create");
            return 1;
        }

        its.it_value.tv_sec = timers[i].period_ns / 1000000000ULL;
        its.it_value.tv_nsec = timers[i].period_ns % 1000000000ULL;
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;

        if (timer_settime(timer_ids[i], 0, &its, NULL) == -1) {
            perror("timer_settime");
            return 1;
        }
    }

    while (!done) {
        usleep(1000);
    }

    for (int i = 0; i < NUM_TIMERS; i++) {
        timer_delete(timer_ids[i]);
    }

    FILE *f = fopen("output_timer_realtime.csv", "w");
    if (!f) {
        perror("fopen");
        return 1;
    }

    for (int i = 0; i < NUM_TIMERS; i++) {
        for (int j = 0; j < timers[i].idx; j++) {
            uint64_t ns = (uint64_t)timers[i].ts[j].tv_sec * 1000000000ULL + timers[i].ts[j].tv_nsec;
            fprintf(f, "%llums;%llu\n",
                    (unsigned long long)(timers[i].period_ns / 1000000ULL),
                    (unsigned long long)ns);
        }
    }

    fclose(f);
    return 0;
}
