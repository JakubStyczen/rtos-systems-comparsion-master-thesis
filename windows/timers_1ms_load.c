#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <mmsystem.h>

#define SAMPLES 100000
#define N_TIMERS 1

volatile float x = 1.001f;

void cpu_load_fpu(void)
{
    for (int i = 0; i < 30; i++)
    {
        for (int j = 0; j < 10000; j++)
        {
            x = x * 1.0001f + 0.0001f;
        }
    }
}

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
        cpu_load_fpu();
    }

    CancelWaitableTimer(timer);
    CloseHandle(timer);
    return 0;
}

int main(void)
{
    QueryPerformanceFrequency(&qpc_freq);

    FILE *f = fopen("windows_jitter_1ms_load.csv", "w");
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
    {
        return 1;
    }

    timeBeginPeriod(1);
    QueryPerformanceFrequency(&qpc_freq);

    timer_data_t t1 = {1};

    HANDLE threads[N_TIMERS];
    threads[0] = CreateThread(NULL, 0, timer_thread, &t1, 0, NULL);

    WaitForMultipleObjects(N_TIMERS, threads, TRUE, INFINITE);

    fprintf(f, "=== SERIES ===\n");

    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "1ms;%lld\n", t1.ts[i].QuadPart);
    }

    fclose(f);

    for (int i = 0; i < N_TIMERS; i++)
        CloseHandle(threads[i]);

    timeEndPeriod(1);
    return 0;
}
