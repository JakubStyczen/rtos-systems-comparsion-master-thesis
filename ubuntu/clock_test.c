#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>

#define UNUSED(x) (void)(x)
#define ARR_LEN 10000
#define BILLION 1000000000L  /* For nanoseconds */

struct t_eventData {
    int myData;
    int ArrCnt;
    double TArr[ARR_LEN];
};

static struct timespec start, stop;

static void tic(void) {
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        perror("clock_gettime start");
    }
}

static double toc(void) {
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
        perror("clock_gettime stop");
    }
    return ((stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / (double)BILLION);
}

static void handler(int sig, siginfo_t *si, void *uc) {
    UNUSED(sig);
    UNUSED(uc);

    struct t_eventData *data = (struct t_eventData *) si->si_value.sival_ptr;

    if (data->ArrCnt < ARR_LEN) {
        double ex = toc();
        tic();
        data->TArr[data->ArrCnt++] = 1000 * ex; // zapis w ms
    }
}

int main(void) {
    int res = 0;
    timer_t timerId = 0;
    struct sigevent sev;
    struct t_eventData eventData = { .myData = 0, .ArrCnt = 0 };
    struct sigaction sa;
    struct itimerspec its;

    memset(&sev, 0, sizeof(sev));
    memset(&sa, 0, sizeof(sa));
    memset(&its, 0, sizeof(its));

    /* specify start delay and interval */
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = BILLION / 10; // 100ms

    /* configure timer event */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &eventData;

    res = timer_create(CLOCK_REALTIME, &sev, &timerId);
    if (res != 0) {
        perror("timer_create");
        return 1;
    }

    /* configure signal handler */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    /* start timer */
    tic(); // start pomiaru czasu
    res = timer_settime(timerId, 0, &its, NULL);
    if (res != 0) {
        perror("timer_settime");
        return 1;
    }

    printf("Press ENTER to Exit\n");
    while (getchar() != '\n') {}
    FILE *f = fopen("output.csv", "w");
    if (f) {
        for (int i = 0; i < eventData.ArrCnt; i++) {
            fprintf(f, "%.3f\n", eventData.TArr[i]);
        }
        fclose(f);
    } else {
        perror("fopen");
    }

    return 0;
}
