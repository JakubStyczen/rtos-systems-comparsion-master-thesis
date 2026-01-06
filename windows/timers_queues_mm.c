#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <mmsystem.h>

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

    FILE *f = fopen("jitter_windows_timerqueue_winmm.csv", "w");
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
    {
    }
    fprintf(f, "1ms;%lld\n", tc.wPeriodMin);
    /* ===== ZMIANA ROZDZIELCZOŚCI ===== */
    timeBeginPeriod(1);
    TIMECAPS tc_2;
    if (timeGetDevCaps(&tc_2, sizeof(TIMECAPS)) != TIMERR_NOERROR)
    {
    }
    fprintf(f, "1ms;%lld\n", tc_2.wPeriodMin);
    QueryPerformanceFrequency(&qpc_freq);

    if (!f)
        return 1;

    /* ===== META ===== */
    fprintf(f, "=== META ===\n");
    fprintf(f, "QPC_FREQ;%lld\n", qpc_freq.QuadPart);
    fprintf(f, "TIMER_TYPE;TimerQueue\n");
    fprintf(f, "WINMM;timeBeginPeriod(1)\n");
    fprintf(f, "=== SERIES ===\n");

    timerQueue = CreateTimerQueue();
    doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    timer_data_t timers[N_TIMERS] = {
        {.period_ms = 1, .index = 0},
        {.period_ms = 2, .index = 0},
        {.period_ms = 5, .index = 0},
        {.period_ms = 10, .index = 0},
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

    /* ===== ZAPIS DANYCH (round-robin) ===== */
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

    timeEndPeriod(1);
    return 0;
}
