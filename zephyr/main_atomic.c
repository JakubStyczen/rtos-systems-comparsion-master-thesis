#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/atomic.h>

#define STACK_SIZE 512
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

static uint32_t start[1000];
static uint32_t finish[1000];
static atomic_t count = ATOMIC_INIT(0);

void print_all_data(void)
{
    int c = atomic_get(&count);
    for (int i = 0; i < c; i++)
    {
        printk("%u;%u\n", start[i], finish[i]);
    }
}

void thread1_func(void *p1, void *p2, void *p3)
{
    while (atomic_get(&count) < 1000)
    {
        int idx = atomic_get(&count);
        if (idx < 1000)
        {
            start[idx] = k_cycle_get_32();
            printk("Wątek 1: mam mutex\n");
        }
        k_sleep(K_USEC(1));
    }
}

void thread2_func(void *p1, void *p2, void *p3)
{
    while (atomic_get(&count) < 1000)
    {
        int idx = atomic_get(&count);
        if (idx < 1000)
        {
            finish[idx] = k_cycle_get_32();
            printk("Wątek 2: mam mutex\n");
            atomic_inc(&count);
        }
        k_sleep(K_USEC(1));
    }
}

void main(void)
{
    printk("Start programu\n");

    k_thread_create(&thread1_data, thread1_stack, STACK_SIZE, thread1_func, NULL, NULL, NULL, PRIORITY,
                    0, K_NO_WAIT);

    k_thread_create(&thread2_data, thread2_stack, STACK_SIZE, thread2_func, NULL, NULL, NULL, PRIORITY,
                    0, K_NO_WAIT);

    while (atomic_get(&count) < 1000)
    {
        k_sleep(K_MSEC(10));
    }

    print_all_data();
}
