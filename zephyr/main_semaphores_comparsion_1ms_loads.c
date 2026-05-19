#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLES 10000
#define REPEAT 10
#define STACK 1024
#define PRIO 1

#define BASE_TICK_US 50

#define P1000_T (1000 / BASE_TICK_US)

volatile float x = 1.001f;

void cpu_load_fpu(void)
{
    for (int i = 0; i < 3000; i++)
    {
        x = x * 1.0001f + 0.0001f;
    }
}

K_SEM_DEFINE(sem_1000, 0, 1);

static uint32_t t_1000[SAMPLES];

static volatile int c1000;

#define TAKER(name, sem, buf, cnt)             \
    void name(void *a, void *b, void *c)       \
    {                                          \
        while (1)                              \
        {                                      \
            k_sem_take(&sem, K_FOREVER);       \
            if (cnt < SAMPLES)                 \
            {                                  \
                buf[cnt++] = k_cycle_get_32(); \
                cpu_load_fpu();                \
            }                                  \
        }                                      \
    }

TAKER(take1000, sem_1000, t_1000, c1000)

K_THREAD_DEFINE(t1000, STACK, take1000, NULL, NULL, NULL, PRIO, 0, 0);

static struct k_timer master_timer;

static void master_timer_cb(struct k_timer *timer_id)
{
    static uint32_t tick = 0;
    tick++;
    if (tick % P1000_T == 0)
    {
        k_sem_give(&sem_1000);
    }
}

static void reset_counters(void)
{
    c1000 = 0;
    tick = 0;
}

int main(void)
{
    k_timer_init(&master_timer, master_timer_cb, NULL);

    for (int r = 0; r < REPEAT; r++)
    {
        reset_counters();

        k_timer_start(&master_timer, K_NO_WAIT, K_USEC(BASE_TICK_US));

        while (c1000 < SAMPLES)
        {
            k_sleep(K_MSEC(10));
        }

        k_timer_stop(&master_timer);

        for (int i = 0; i < SAMPLES; i++)
        {
            printk("1000us;%u\n", t_1000[i]);
        }
    }
    return 0;
}