#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define SAMPLES 1000
#define N_TIMERS 5

typedef struct
{
    int period_ms;
    volatile LONG index;
    LARGE_INTEGER ts[SAMPLES];
} timer_data_t;

LARGE_INTEGER qpc_freq;
HANDLE timerQueue;
HANDLE doneEvent;

volatile LONG finishedTimers = 0;

VOID CALLBACK timer_callback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    timer_data_t *d = (timer_data_t *)lpParameter;

    LONG i = InterlockedIncrement(&d->index) - 1;
    if (i < SAMPLES)
    {
        QueryPerformanceCounter(&d->ts[i]);
    }
    else
    {
        if (InterlockedIncrement(&finishedTimers) == N_TIMERS)
        {
            SetEvent(doneEvent);
        }
    }
}

int main(void)
{
    QueryPerformanceFrequency(&qpc_freq);

    FILE *f = fopen("jitter_windows_timerqueue.csv", "w");
    if (!f)
        return 1;

    fprintf(f, "=== META ===\n");
    fprintf(f, "QPC_FREQ;%lld\n", qpc_freq.QuadPart);
    fprintf(f, "NOTE;Windows Timer Queue, no winmm\n");
    fprintf(f, "=== SERIES ===\n");

    timerQueue = CreateTimerQueue();
    doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    timer_data_t timers[N_TIMERS] = {
        {.period_ms = 1, .index = 0},
        {.period_ms = 2, .index = 0},
        {.period_ms = 5, .index = 0},
        {.period_ms = 20, .index = 0},
        {.period_ms = 100, .index = 0},
    };

    HANDLE timerHandles[N_TIMERS];

    for (int i = 0; i < N_TIMERS; i++)
    {
        CreateTimerQueueTimer(
            &timerHandles[i],
            timerQueue,
            timer_callback,
            &timers[i],
            timers[i].period_ms,
            timers[i].period_ms,
            WT_EXECUTEDEFAULT);
    }

    WaitForSingleObject(doneEvent, INFINITE);

    /* zapis round-robin jak wcześniej */
    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "1ms;%lld\n", timers[0].ts[i].QuadPart);
        fprintf(f, "2ms;%lld\n", timers[1].ts[i].QuadPart);
        fprintf(f, "5ms;%lld\n", timers[2].ts[i].QuadPart);
        fprintf(f, "10ms;%lld\n", timers[3].ts[i].QuadPart);
        fprintf(f, "100ms;%lld\n", timers[4].ts[i].QuadPart);
    }

    fclose(f);

    DeleteTimerQueueEx(timerQueue, INVALID_HANDLE_VALUE);
    CloseHandle(doneEvent);

    return 0;
}
