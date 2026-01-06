#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <mmsystem.h>

#define SAMPLES 5000
#define N_TIMERS 5

typedef struct
{
    int period_ms;
    LARGE_INTEGER ts[SAMPLES];
} timer_data_t;

LARGE_INTEGER qpc_freq;

DWORD WINAPI timer_thread(void *arg)
{
    timer_data_t *d = (timer_data_t *)arg;

    HANDLE timer = CreateWaitableTimer(NULL, FALSE, NULL);
    LARGE_INTEGER due;
    due.QuadPart = -(LONGLONG)d->period_ms * 10000;

    SetWaitableTimer(timer, &due, d->period_ms, NULL, NULL, FALSE);

    for (int i = 0; i < SAMPLES; i++)
    {
        WaitForSingleObject(timer, INFINITE);
        QueryPerformanceCounter(&d->ts[i]);
    }

    CancelWaitableTimer(timer);
    CloseHandle(timer);
    return 0;
}

int main(void)
{
    QueryPerformanceFrequency(&qpc_freq);

    FILE *f = fopen("jitter_windows_series.csv", "w");
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
    {
    }
    // fprintf(f, "1ms;%lld\n", tc.wPeriodMin);

    fprintf(f, "=== META ===\n");
    fprintf(f, "QPC_FREQ;%lld\n", qpc_freq.QuadPart);
    fprintf(f, "=== SERIES ===\n");

    timeBeginPeriod(1);
    QueryPerformanceFrequency(&qpc_freq);

    timer_data_t t1 = {1};
    timer_data_t t2 = {2};
    timer_data_t t5 = {5};
    timer_data_t t10 = {10};
    timer_data_t t100 = {100};

    HANDLE threads[N_TIMERS];
    threads[0] = CreateThread(NULL, 0, timer_thread, &t1, 0, NULL);
    threads[1] = CreateThread(NULL, 0, timer_thread, &t2, 0, NULL);
    threads[2] = CreateThread(NULL, 0, timer_thread, &t5, 0, NULL);
    threads[3] = CreateThread(NULL, 0, timer_thread, &t10, 0, NULL);
    threads[4] = CreateThread(NULL, 0, timer_thread, &t100, 0, NULL);

    WaitForMultipleObjects(N_TIMERS, threads, TRUE, INFINITE);

    fprintf(f, "=== SERIES ===\n");

    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "1ms;%lld\n", t1.ts[i].QuadPart);
        fprintf(f, "2ms;%lld\n", t2.ts[i].QuadPart);
        fprintf(f, "5ms;%lld\n", t5.ts[i].QuadPart);
        fprintf(f, "10ms;%lld\n", t10.ts[i].QuadPart);
        fprintf(f, "100ms;%lld\n", t100.ts[i].QuadPart);
    }

    fclose(f);

    for (int i = 0; i < N_TIMERS; i++)
        CloseHandle(threads[i]);

    timeEndPeriod(1);
    return 0;
}
