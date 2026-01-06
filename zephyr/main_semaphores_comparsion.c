#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLES 1000
#define STACK 1024
#define PRIO 1

/* === periods === */
#define P50 K_USEC(50)
#define P250 K_USEC(250)
#define P500 K_USEC(500)
#define P1000 K_USEC(1000)
#define P5000 K_USEC(5000)
#define P10000 K_USEC(10000)

/* === semaphores === */
K_SEM_DEFINE(sem_50, 0, 1);
K_SEM_DEFINE(sem_250, 0, 1);
K_SEM_DEFINE(sem_500, 0, 1);
K_SEM_DEFINE(sem_1000, 0, 1);
K_SEM_DEFINE(sem_5000, 0, 1);
K_SEM_DEFINE(sem_10000, 0, 1);

/* === time buffers === */
static uint32_t t_50[SAMPLES];
static uint32_t t_250[SAMPLES];
static uint32_t t_500[SAMPLES];
static uint32_t t_1000[SAMPLES];
static uint32_t t_5000[SAMPLES];
static uint32_t t_10000[SAMPLES];

static int c50, c250, c500, c1000, c5000, c10000;

/* === generic taker === */
#define TAKER(name, sem, buf, cnt)         \
    void name(void *a, void *b, void *c)   \
    {                                      \
        while (cnt < SAMPLES)              \
        {                                  \
            k_sem_take(&sem, K_FOREVER);   \
            buf[cnt++] = k_cycle_get_32(); \
        }                                  \
    }

TAKER(take50, sem_50, t_50, c50)
TAKER(take250, sem_250, t_250, c250)
TAKER(take500, sem_500, t_500, c500)
TAKER(take1000, sem_1000, t_1000, c1000)
TAKER(take5000, sem_5000, t_5000, c5000)
TAKER(take10000, sem_10000, t_10000, c10000)

/* === generic giver === */
#define GIVER(name, sem, period)         \
    void name(void *a, void *b, void *c) \
    {                                    \
        while (1)                        \
        {                                \
            k_sleep(period);             \
            k_sem_give(&sem);            \
        }                                \
    }

GIVER(give50, sem_50, P50)
GIVER(give250, sem_250, P250)
GIVER(give500, sem_500, P500)
GIVER(give1000, sem_1000, P1000)
GIVER(give5000, sem_5000, P5000)
GIVER(give10000, sem_10000, P10000)

/* === threads === */
K_THREAD_DEFINE(t50, STACK, take50, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t250, STACK, take250, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t500, STACK, take500, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t1000, STACK, take1000, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t5000, STACK, take5000, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t10000, STACK, take10000, NULL, NULL, NULL, PRIO, 0, 0);

K_THREAD_DEFINE(g50, STACK, give50, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(g250, STACK, give250, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(g500, STACK, give500, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(g1000, STACK, give1000, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(g5000, STACK, give5000, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(g10000, STACK, give10000, NULL, NULL, NULL, PRIO, 0, 0);

int main(void)
{
    while (c10000 < SAMPLES)
    {
        k_sleep(K_MSEC(100));
    }

    for (int i = 0; i < SAMPLES; i++)
    {
        printk("50us;%u\n", t_50[i]);
        printk("250us;%u\n", t_250[i]);
        printk("500us;%u\n", t_500[i]);
        printk("1000us;%u\n", t_1000[i]);
        printk("5000us;%u\n", t_5000[i]);
        printk("10000us;%u\n", t_10000[i]);
    }

    return 0;
}
