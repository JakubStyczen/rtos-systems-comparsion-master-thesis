#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLES 1000
#define REPEATS 2

static uint64_t time_100[SAMPLES], time_200[SAMPLES], time_500[SAMPLES];
static uint64_t time_1000[SAMPLES], time_5000[SAMPLES], time_10000[SAMPLES];

static int count_100 = 0, count_200 = 0, count_500 = 0;
static int count_1000 = 0, count_5000 = 0, count_10000 = 0;

static bool done_100 = false, done_200 = false, done_500 = false;
static bool done_1000 = false, done_5000 = false, done_10000 = false;

static int repeat = 0;

extern void fn_clock_100us(struct k_timer *timer_id)
{
    if (count_100 < SAMPLES)
        time_100[count_100++] = k_cycle_get_64();
    if (count_100 == SAMPLES)
        done_100 = true;
}

extern void fn_clock_200us(struct k_timer *timer_id)
{
    if (count_200 < SAMPLES)
        time_200[count_200++] = k_cycle_get_64();
    if (count_200 == SAMPLES)
        done_200 = true;
}

extern void fn_clock_500us(struct k_timer *timer_id)
{
    if (count_500 < SAMPLES)
        time_500[count_500++] = k_cycle_get_64();
    if (count_500 == SAMPLES)
        done_500 = true;
}

extern void fn_clock_1000us(struct k_timer *timer_id)
{
    if (count_1000 < SAMPLES)
        time_1000[count_1000++] = k_cycle_get_64();
    if (count_1000 == SAMPLES)
        done_1000 = true;
}

extern void fn_clock_5000us(struct k_timer *timer_id)
{
    if (count_5000 < SAMPLES)
        time_5000[count_5000++] = k_cycle_get_64();
    if (count_5000 == SAMPLES)
        done_5000 = true;
}

extern void fn_clock_10000us(struct k_timer *timer_id)
{
    if (count_10000 < SAMPLES)
        time_10000[count_10000++] = k_cycle_get_64();
    if (count_10000 == SAMPLES)
        done_10000 = true;
}

K_TIMER_DEFINE(my_timer_100us, fn_clock_100us, NULL);
K_TIMER_DEFINE(my_timer_200us, fn_clock_200us, NULL);
K_TIMER_DEFINE(my_timer_500us, fn_clock_500us, NULL);
K_TIMER_DEFINE(my_timer_1000us, fn_clock_1000us, NULL);
K_TIMER_DEFINE(my_timer_5000us, fn_clock_5000us, NULL);
K_TIMER_DEFINE(my_timer_10000us, fn_clock_10000us, NULL);

void logging_thread(void)
{
    while (repeat < REPEATS)
    {
        if (done_100 && done_200 && done_500 && done_1000 && done_5000 && done_10000)
        {

            printk("=== SERIES %d ===\n", repeat + 1);
            for (int i = 0; i < SAMPLES; i++)
            {
                printk("100us;%llu\n200us;%llu\n500us;%llu\n1000us;%llu\n5000us;%llu\n10000us;%llu\n",
                       time_100[i], time_200[i], time_500[i],
                       time_1000[i], time_5000[i], time_10000[i]);
            }

            repeat++;

            // reset counters and flags for next series
            count_100 = count_200 = count_500 = 0;
            count_1000 = count_5000 = count_10000 = 0;
            done_100 = done_200 = done_500 = false;
            done_1000 = done_5000 = done_10000 = false;

            // restart timers
            k_timer_start(&my_timer_100us, K_USEC(100), K_USEC(100));
            k_timer_start(&my_timer_200us, K_USEC(200), K_USEC(200));
            k_timer_start(&my_timer_500us, K_USEC(500), K_USEC(500));
            k_timer_start(&my_timer_1000us, K_USEC(1000), K_USEC(1000));
            k_timer_start(&my_timer_5000us, K_USEC(5000), K_USEC(5000));
            k_timer_start(&my_timer_10000us, K_USEC(10000), K_USEC(10000));
        }
        k_msleep(10);
    }

    printk("All series completed.\n");
    while (1)
        k_sleep(K_FOREVER);
}

K_THREAD_DEFINE(log_thread, 8192, logging_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
    // start first series
    k_timer_start(&my_timer_100us, K_USEC(100), K_USEC(100));
    k_timer_start(&my_timer_200us, K_USEC(200), K_USEC(200));
    k_timer_start(&my_timer_500us, K_USEC(500), K_USEC(500));
    k_timer_start(&my_timer_1000us, K_USEC(1000), K_USEC(1000));
    k_timer_start(&my_timer_5000us, K_USEC(5000), K_USEC(5000));
    k_timer_start(&my_timer_10000us, K_USEC(10000), K_USEC(10000));

    printk("Timers started\n");

    return 0;
}
