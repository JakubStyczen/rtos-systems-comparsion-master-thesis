#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define SAMPLES 1000

HANDLE hMutex;
static LONG count = 0;

LARGE_INTEGER start[SAMPLES];
LARGE_INTEGER finish[SAMPLES];
LARGE_INTEGER freq;

DWORD WINAPI thread1_func(LPVOID arg)
{
    while (count < SAMPLES)
    {
        DWORD r = WaitForSingleObject(hMutex, 1);
        if (r == WAIT_OBJECT_0)
        {

            {
                printf("Wątek 1: mam mutex %ld\n", count);
                QueryPerformanceCounter(&start[count]);
                ReleaseMutex(hMutex);
            }
        }
    }
    return 0;
}

DWORD WINAPI thread2_func(LPVOID arg)
{
    while (count < SAMPLES)
    {
        DWORD r = WaitForSingleObject(hMutex, 1);
        if (r == WAIT_OBJECT_0)
        {
            printf("Wątek 2: mam mutex %ld\n", count);
            QueryPerformanceCounter(&finish[count]);
            count++;
            ReleaseMutex(hMutex);
        }
    }
    return 0;
}

int main(void)
{
    HANDLE t1, t2;

    QueryPerformanceFrequency(&freq);
    hMutex = CreateMutex(NULL, FALSE, NULL);

    printf("Start programu\n");

    t1 = CreateThread(NULL, 0, thread1_func, NULL, 0, NULL);
    t2 = CreateThread(NULL, 0, thread2_func, NULL, 0, NULL);

    WaitForSingleObject(t1, INFINITE);
    WaitForSingleObject(t2, INFINITE);

    FILE *f;
    f = fopen("mutex_switch.csv", "w");
    if (!f)
        return -1;

    for (int i = 0; i < SAMPLES; i++)
    {
        fprintf(f, "%lld;%lld\n",
                start[i].QuadPart,
                finish[i].QuadPart);
    }

    fclose(f);

    // for (int i = 0; i < SAMPLES; i++)
    // {
    //     printf("%lld;%lld\n",
    //            start[i].QuadPart,
    //            finish[i].QuadPart);
    // }

    CloseHandle(t1);
    CloseHandle(t2);
    CloseHandle(hMutex);

    return 0;
}
