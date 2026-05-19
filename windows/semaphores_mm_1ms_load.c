#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>

#pragma comment(lib, "winmm.lib")

#define SAMPLES 100000

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

HANDLE sem_1, sem_2, sem_5, sem_10, sem_100;

/* === buffers === */
LARGE_INTEGER t1[SAMPLES];
// LARGE_INTEGER t2[SAMPLES];
// LARGE_INTEGER t5[SAMPLES];
// LARGE_INTEGER t10[SAMPLES];
// LARGE_INTEGER t100[SAMPLES];

volatile LONG c1 = 0, c2 = 0, c5 = 0, c10 = 0, c100 = 0;
MMRESULT timer_id;

/* === taker macro === */
#define TAKER(name, sem, buf, cnt)                   \
    DWORD WINAPI name(LPVOID arg)                    \
    {                                                \
        while (1)                                    \
        {                                            \
            WaitForSingleObject(sem, INFINITE);      \
            LONG i = InterlockedIncrement(&cnt) - 1; \
            if (i < SAMPLES)                         \
            {                                        \
                QueryPerformanceCounter(&buf[i]);    \
                cpu_load_fpu();                      \
            }                                        \
            else if (c1 >= SAMPLES)                  \
                break;                               \
        }                                            \
        return 0;                                    \
    }

TAKER(take1, sem_1, t1, c1)

void CALLBACK mm_timer_cb(UINT uID, UINT uMsg, DWORD_PTR dwUser,
                          DWORD_PTR dw1, DWORD_PTR dw2)
{
    static DWORD tick = 0;
    tick++;

    ReleaseSemaphore(sem_1, 1, NULL);

    if (c1 >= SAMPLES)
    {
        timeKillEvent(timer_id);

        ReleaseSemaphore(sem_1, SAMPLES, NULL);
    }
}

int main(void)
{
    HANDLE th1, th2, th5, th10, th100;
    FILE *f;

    sem_1 = CreateSemaphore(NULL, 0, SAMPLES, NULL);

    th1 = CreateThread(NULL, 0, take1, NULL, 0, NULL);

    timeBeginPeriod(1);

    timer_id = timeSetEvent(
        1,
        0,
        mm_timer_cb,
        0,
        TIME_PERIODIC | TIME_CALLBACK_FUNCTION);

    WaitForSingleObject(th1, INFINITE);

    timeEndPeriod(1);

    f = fopen("semaphore_mm_1_ms_load.csv", "w");
    fprintf(f, "==========\n");

    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "1ms;%lld\n", t1[i].QuadPart);
    }

    fclose(f);
    return 0;
}
