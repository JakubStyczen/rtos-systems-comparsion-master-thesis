#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define SAMPLES 1000

volatile LONG count = 0;

LARGE_INTEGER start[SAMPLES];
LARGE_INTEGER finish[SAMPLES];
LARGE_INTEGER freq;

DWORD WINAPI thread1_func(LPVOID arg)
{
    while (InterlockedCompareExchange(&count, 0, 0) < SAMPLES)
    {
        LONG idx = InterlockedCompareExchange(&count, 0, 0);
        if (idx < SAMPLES)
        {
            QueryPerformanceCounter(&start[idx]);
            printf("Wątek 1: mam mutex %ld\n", idx);
        }
        Sleep(1); /* odpowiednik k_sleep(K_MSEC(1)) */
    }
    return 0;
}

DWORD WINAPI thread2_func(LPVOID arg)
{
    while (InterlockedCompareExchange(&count, 0, 0) < SAMPLES)
    {
        LONG idx = InterlockedCompareExchange(&count, 0, 0);
        if (idx < SAMPLES)
        {
            QueryPerformanceCounter(&finish[idx]);
            printf("Wątek 2: mam mutex %ld\n", idx);
            InterlockedIncrement(&count);
        }
        Sleep(1);
    }
    return 0;
}

int main(void)
{
    HANDLE t1, t2;
    FILE *f;

    QueryPerformanceFrequency(&freq);

    t1 = CreateThread(NULL, 0, thread1_func, NULL, 0, NULL);
    t2 = CreateThread(NULL, 0, thread2_func, NULL, 0, NULL);

    // SetThreadPriority(t1, THREAD_PRIORITY_HIGHEST);
    // SetThreadPriority(t2, THREAD_PRIORITY_HIGHEST);
    // SetThreadAffinityMask(t1, 1);
    // SetThreadAffinityMask(t2, 1);

    WaitForSingleObject(t1, INFINITE);
    WaitForSingleObject(t2, INFINITE);

    f = fopen("atomic_switch.csv", "w");

    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "%lld;%lld\n",
                start[i].QuadPart,
                finish[i].QuadPart);
    }

    fclose(f);

    CloseHandle(t1);
    CloseHandle(t2);

    return 0;
}
