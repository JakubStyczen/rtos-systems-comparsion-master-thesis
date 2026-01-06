#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLES 1000
#define STACK 1024
#define PRIO 1

/* === base clock === */
#define BASE_TICK_US 50

/* === periods in ticks === */
#define P50_T (100 / BASE_TICK_US)      // 2
#define P250_T (200 / BASE_TICK_US)     // 4
#define P500_T (500 / BASE_TICK_US)     // 10
#define P1000_T (1000 / BASE_TICK_US)   // 20
#define P5000_T (5000 / BASE_TICK_US)   // 100
#define P10000_T (10000 / BASE_TICK_US) // 200

/* === semaphores === */
K_SEM_DEFINE(sem_50, 0, 1);
K_SEM_DEFINE(sem_250, 0, 1);
K_SEM_DEFINE(sem_500, 0, 1);
K_SEM_DEFINE(sem_1000, 0, 1);
K_SEM_DEFINE(sem_5000, 0, 1);
K_SEM_DEFINE(sem_10000, 0, 1);

/* === buffers === */
static uint32_t t_50[SAMPLES];
static uint32_t t_250[SAMPLES];
static uint32_t t_500[SAMPLES];
static uint32_t t_1000[SAMPLES];
static uint32_t t_5000[SAMPLES];
static uint32_t t_10000[SAMPLES];

static int c50, c250, c500, c1000, c5000, c10000;

/* === takers (bez zmian) === */
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

/* === threads === */
K_THREAD_DEFINE(t50, STACK, take50, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t250, STACK, take250, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t500, STACK, take500, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t1000, STACK, take1000, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t5000, STACK, take5000, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t10000, STACK, take10000, NULL, NULL, NULL, PRIO, 0, 0);

/* === master clock === */
static struct k_timer master_timer;

static void master_timer_cb(struct k_timer *dummy)
{
    static uint32_t tick = 0;
    tick++;

    if (tick % P50_T == 0)
    {
        k_sem_give(&sem_50);
    }
    if (tick % P250_T == 0)
    {
        k_sem_give(&sem_250);
    }
    if (tick % P500_T == 0)
    {
        k_sem_give(&sem_500);
    }
    if (tick % P1000_T == 0)
    {
        k_sem_give(&sem_1000);
    }
    if (tick % P5000_T == 0)
    {
        k_sem_give(&sem_5000);
    }
    if (tick % P10000_T == 0)
    {
        k_sem_give(&sem_10000);
    }
}

int main(void)
{
    k_timer_init(&master_timer, master_timer_cb, NULL);
    k_timer_start(&master_timer, K_NO_WAIT, K_USEC(BASE_TICK_US));

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
