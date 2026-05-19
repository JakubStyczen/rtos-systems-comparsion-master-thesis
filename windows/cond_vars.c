#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define SAMPLES 100000

CRITICAL_SECTION cs;
CONDITION_VARIABLE cv1;
CONDITION_VARIABLE cv2;

int turn = 1;
int count = 0;

LARGE_INTEGER start[SAMPLES];
LARGE_INTEGER finish[SAMPLES];
LARGE_INTEGER freq;

DWORD WINAPI thread1_func(LPVOID arg)
{
    while (1)
    {
        EnterCriticalSection(&cs);

        while (turn != 1 && count < SAMPLES)
            SleepConditionVariableCS(&cv1, &cs, INFINITE);

        if (count >= SAMPLES)
        {
            LeaveCriticalSection(&cs);
            break;
        }

        QueryPerformanceCounter(&start[count]);
        turn = 2;

        WakeConditionVariable(&cv2);
        LeaveCriticalSection(&cs);
    }
    return 0;
}

DWORD WINAPI thread2_func(LPVOID arg)
{
    while (1)
    {
        EnterCriticalSection(&cs);

        while (turn != 2 && count < SAMPLES)
            SleepConditionVariableCS(&cv2, &cs, INFINITE);

        if (count >= SAMPLES)
        {
            LeaveCriticalSection(&cs);
            break;
        }

        QueryPerformanceCounter(&finish[count]);
        count++;
        turn = 1;

        WakeConditionVariable(&cv1);
        LeaveCriticalSection(&cs);
    }
    return 0;
}

int main(void)
{
    HANDLE t1, t2;

    QueryPerformanceFrequency(&freq);

    InitializeCriticalSection(&cs);
    InitializeConditionVariable(&cv1);
    InitializeConditionVariable(&cv2);

    t1 = CreateThread(NULL, 0, thread1_func, NULL, 0, NULL);
    t2 = CreateThread(NULL, 0, thread2_func, NULL, 0, NULL);

    WaitForSingleObject(t1, INFINITE);
    WaitForSingleObject(t2, INFINITE);

    FILE *f = fopen("windows_condvar_switch_printf.csv", "w");
    for (int i = 0; i < SAMPLES; i++)
        fprintf(f, "%lld;%lld\n",
                start[i].QuadPart,
                finish[i].QuadPart);

    fclose(f);

    CloseHandle(t1);
    CloseHandle(t2);
    DeleteCriticalSection(&cs);

    return 0;
}
