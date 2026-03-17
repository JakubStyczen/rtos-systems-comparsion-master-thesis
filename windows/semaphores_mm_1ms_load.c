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

/* === semaphores === */
HANDLE sem_1, sem_2, sem_5, sem_10, sem_100;

/* === buffers === */
LARGE_INTEGER t1[SAMPLES];
// LARGE_INTEGER t2[SAMPLES];
// LARGE_INTEGER t5[SAMPLES];
// LARGE_INTEGER t10[SAMPLES];
// LARGE_INTEGER t100[SAMPLES];

volatile LONG c1 = 0, c2 = 0, c5 = 0, c10 = 0, c100 = 0;

/* === timer id === */
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
// TAKER(take2, sem_2, t2, c2)
// TAKER(take5, sem_5, t5, c5)
// TAKER(take10, sem_10, t10, c10)
// TAKER(take100, sem_100, t100, c100)

/* === multimedia timer callback (1 ms base) === */
void CALLBACK mm_timer_cb(UINT uID, UINT uMsg, DWORD_PTR dwUser,
                          DWORD_PTR dw1, DWORD_PTR dw2)
{
    static DWORD tick = 0;
    tick++;

    ReleaseSemaphore(sem_1, 1, NULL);

    // if ((tick % 2) == 0)
    //     ReleaseSemaphore(sem_2, 1, NULL);

    // if ((tick % 5) == 0)
    //     ReleaseSemaphore(sem_5, 1, NULL);

    // if ((tick % 10) == 0)
    //     ReleaseSemaphore(sem_10, 1, NULL);

    // if ((tick % 100) == 0)
    //     ReleaseSemaphore(sem_100, 1, NULL);

    if (c1 >= SAMPLES)
    {
        timeKillEvent(timer_id);

        ReleaseSemaphore(sem_1, SAMPLES, NULL);
        // ReleaseSemaphore(sem_2, SAMPLES, NULL);
        // ReleaseSemaphore(sem_5, SAMPLES, NULL);
        // ReleaseSemaphore(sem_10, SAMPLES, NULL);
        // ReleaseSemaphore(sem_100, SAMPLES, NULL);
    }
}

int main(void)
{
    HANDLE th1, th2, th5, th10, th100;
    FILE *f;

    sem_1 = CreateSemaphore(NULL, 0, SAMPLES, NULL);
    // sem_2 = CreateSemaphore(NULL, 0, SAMPLES, NULL);
    // sem_5 = CreateSemaphore(NULL, 0, SAMPLES, NULL);
    // sem_10 = CreateSemaphore(NULL, 0, SAMPLES, NULL);
    // sem_100 = CreateSemaphore(NULL, 0, SAMPLES, NULL);

    th1 = CreateThread(NULL, 0, take1, NULL, 0, NULL);
    // th2 = CreateThread(NULL, 0, take2, NULL, 0, NULL);
    // th5 = CreateThread(NULL, 0, take5, NULL, 0, NULL);
    // th10 = CreateThread(NULL, 0, take10, NULL, 0, NULL);
    // th100 = CreateThread(NULL, 0, take100, NULL, 0, NULL);

    timeBeginPeriod(1);

    timer_id = timeSetEvent(
        1,
        0,
        mm_timer_cb,
        0,
        TIME_PERIODIC | TIME_CALLBACK_FUNCTION);

    WaitForSingleObject(th1, INFINITE);
    // WaitForSingleObject(th2, INFINITE);
    // WaitForSingleObject(th5, INFINITE);
    // WaitForSingleObject(th10, INFINITE);
    // WaitForSingleObject(th100, INFINITE);

    timeEndPeriod(1);

    f = fopen("semaphore_mm_1_ms_load.csv", "w");
    fprintf(f, "==========\n");

    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "1ms;%lld\n", t1[i].QuadPart);
        // fprintf(f, "2ms;%lld\n", t2[i].QuadPart);
        // fprintf(f, "5ms;%lld\n", t5[i].QuadPart);
        // fprintf(f, "10ms;%lld\n", t10[i].QuadPart);
        // fprintf(f, "100ms;%lld\n", t100[i].QuadPart);
    }

    fclose(f);
    return 0;
}
