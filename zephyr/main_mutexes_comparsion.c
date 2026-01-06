#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define PRIORITY 5

K_MUTEX_DEFINE(my_mutex);

K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

static uint32_t start[1000];
static uint32_t finish[1000];
static int count = 0;

void print_all_data(void)
{
    for (int i = 0; i < count; i++)
    {
        printk("%u;%u\n", start[i], finish[i]);
    }
}

void thread1_func(void *p1, void *p2, void *p3)
{
    while (count < 1000)
    {
        if (k_mutex_lock(&my_mutex, K_MSEC(1)) == 0)
        {
            // printk("Wątek 1: mam mutex\n");
            // k_sleep(K_MSEC(200));
            start[count] = k_cycle_get_32();
            // printk("Wątek 1: zwalniam mutex\n");
            k_mutex_unlock(&my_mutex);
        }
        // k_sleep(K_MSEC(200));
    }
}

void thread2_func(void *p1, void *p2, void *p3)
{
    while (count < 1000)
    {
        if (k_mutex_lock(&my_mutex, K_MSEC(1)) == 0)
        {
            // printk("Wątek 2: mam mutex\n");
            // k_sleep(K_MSEC(200));
            finish[count] = k_cycle_get_32();
            count++;
            // printk("Wątek 2: zwalniam mutex\n");
            k_mutex_unlock(&my_mutex);
        }
        // k_sleep(K_MSEC(200));
    }
}

void main(void)
{
    printk("Start programu\n");

    k_thread_create(&thread1_data, thread1_stack, STACK_SIZE, thread1_func, NULL, NULL, NULL, 0,
                    0, K_NO_WAIT);

    k_thread_create(&thread2_data, thread2_stack, STACK_SIZE, thread2_func, NULL, NULL, NULL, 0,
                    0, K_NO_WAIT);

    while (count != 1000)
    {
        k_sleep(K_MSEC(100));
    }

    print_all_data();
}
