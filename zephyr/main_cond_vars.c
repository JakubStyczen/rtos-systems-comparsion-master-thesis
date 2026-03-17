#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define PRIORITY 5
#define SAMPLES 20000

K_MUTEX_DEFINE(my_mutex);
K_CONDVAR_DEFINE(cv1);
K_CONDVAR_DEFINE(cv2);

K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

static uint32_t start[SAMPLES];
static uint32_t finish[SAMPLES];

static int count = 0;
static int turn = 1;

void print_all_data(void)
{
    for (int i = 0; i < SAMPLES; i++)
    {
        printk("%u;%u\n", start[i], finish[i]);
    }
}

void thread1_func(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_mutex_lock(&my_mutex, K_FOREVER);

        while (turn != 1 && count < SAMPLES)
        {
            k_condvar_wait(&cv1, &my_mutex, K_FOREVER);
        }

        if (count >= SAMPLES)
        {
            k_mutex_unlock(&my_mutex);
            break;
        }
        start[count] = k_cycle_get_32();
        turn = 2;
        printk("Thread 1");
        k_condvar_signal(&cv2);
        k_mutex_unlock(&my_mutex);
    }
}

void thread2_func(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_mutex_lock(&my_mutex, K_FOREVER);

        while (turn != 2 && count < SAMPLES)
        {
            k_condvar_wait(&cv2, &my_mutex, K_FOREVER);
        }

        if (count >= SAMPLES)
        {
            k_mutex_unlock(&my_mutex);
            break;
        }

        finish[count] = k_cycle_get_32();

        count++;
        turn = 1;
        printk("Thread 1");
        k_condvar_signal(&cv1);
        k_mutex_unlock(&my_mutex);
    }
}

void main(void)
{
    printk("Start programu\n");

    k_thread_create(&thread1_data, thread1_stack, STACK_SIZE, thread1_func, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&thread2_data, thread2_stack, STACK_SIZE, thread2_func, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    while (count < SAMPLES)
    {
        k_sleep(K_MSEC(100));
    }

    print_all_data();
}
