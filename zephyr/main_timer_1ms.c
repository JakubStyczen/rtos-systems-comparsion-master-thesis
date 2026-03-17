#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLES 10000
#define REPEATS 10

static uint64_t time_1000[SAMPLES];
static int count_1000 = 0;
static bool done_1000 = false;

static int repeat = 0;

volatile float x = 1.001f;

void cpu_load_fpu(void)
{
    for (int i = 0; i < 5000; i++)
    {
        x = x * 1.0001f + 0.0001f;
    }
}

void fn_clock_1000us(struct k_timer *timer_id)
{
    if (count_1000 < SAMPLES)
    {
        time_1000[count_1000++] = k_cycle_get_64();
        cpu_load_fpu();
    }

    if (count_1000 == SAMPLES)
    {
        done_1000 = true;
        k_timer_stop(timer_id); // zatrzymujemy timer po zebraniu próbek
    }
}

K_TIMER_DEFINE(my_timer_1000us, fn_clock_1000us, NULL);

void logging_thread(void)
{
    while (repeat < REPEATS)
    {

        if (done_1000)
        {

            printk("=== SERIES %d ===\n", repeat + 1);

            for (int i = 0; i < SAMPLES; i++)
            {
                printk("1000us;%llu\n", time_1000[i]);
            }

            repeat++;

            // reset na kolejną serię
            count_1000 = 0;
            done_1000 = false;

            k_timer_start(&my_timer_1000us, K_USEC(1000), K_USEC(1000));
        }

        k_msleep(10);
    }

    printk("All series completed.\n");

    while (1)
    {
        k_sleep(K_FOREVER);
    }
}

K_THREAD_DEFINE(log_thread, 8192, logging_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
    k_timer_start(&my_timer_1000us, K_USEC(1000), K_USEC(1000));

    printk("1000us timer started\n");

    return 0;
}
